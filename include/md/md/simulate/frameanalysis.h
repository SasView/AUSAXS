#pragma once

#include <md/programs/all.h>
#include <md/programs/saxsmdrun.h>
#include <md/programs/mdrun/MDRunResult.h>
#include <md/simulate/GMXOptions.h>
#include <md/utility/Exceptions.h>
#include <md/utility/Protein.h>
#include <md/utility/files/MDPCreator.h>

#include <math.h>

namespace md {
    std::vector<SAXSOutput> frameanalysis(SAXSOptions& options) {
        if (!options.molecule.top.exists()) {throw except::io_error("simulate_saxs: The topology file does not exist.");}

        // get data from sims
        if (options.molecule.job == nullptr) {throw except::unexpected("gmx::simulate: No molecule simulation was created.");}
        if (options.buffer.job == nullptr) {throw except::unexpected("gmx::simulate: No buffer was created.");}
        options.molecule.job->submit();
        options.buffer.job->submit();
        options.molecule.job->wait();
        options.buffer.job->wait();
        auto[molgro, moledr, molxtc] = options.molecule.job->result();
        auto[bufgro, bufedr, bufxtc] = options.buffer.job->result();

        //##################################//
        //###           GLOBALS          ###//
        //##################################//
        Folder output = options.output + "saxs/";
        Folder protein_path = output + "protein/";
        Folder buffer_path = output + "buffer/";
        Folder mdp_folder = output + "mdp/";
        Folder prod_folder = output + "prod/";

        TOPFile moltop(protein_path + "topol.top");
        TOPFile buftop(buffer_path + "topol.top");
        if (!moltop.exists() || !buftop.exists()) {
            options.molecule.top.copy(moltop.parent_path() + "/");
            options.buffer.top.copy(buftop.parent_path() + "/");
        }

        //##################################//
        //###        INDEX FILES         ###//
        //##################################//
        // we want to always have the group Water_and_ions, but it is only generated automatically if there are actual ions present in the system. 
        // since this is not guaranteed, we have to generate it manually
        NDXFile molindex(protein_path + "index.ndx");
        NDXFile bufindex(buffer_path + "index.ndx");

        if (!molindex.exists()) {
            make_ndx(molgro)
                .output(molindex)
            .run();
            if (!molindex.contains("RealWater_and_Ions")) {
                auto[tmp] = select(options.molecule.gro)
                    .output("tmp.ndx")
                    .define("\"RealWater_and_Ions\" name \"OW\" or name \"HW1\" or name \"HW2\" or name \"HW3\" or group \"Ion\"")
                .run();

                molindex.append_file(tmp);
                tmp.remove();
            }
        }

        if (!bufindex.exists()) {
            make_ndx(bufgro)
                .output(bufindex)
            .run();
            if (!bufindex.contains("RealWater")) {
                auto[tmp] = select(options.buffer.gro)
                    .output("tmp.ndx")
                    .define("\"RealWater\" name \"OW\" or name \"HW1\" or name \"HW2\" or name \"HW3\"")
                .run();

                bufindex.append_file(tmp);
                tmp.remove();
            }
        }

        //##################################//
        //###         ENVELOPE           ###//
        //##################################//
        GROFile envgro(protein_path + "envelope-ref.gro");
        PYFile envpy(protein_path + "envelope.py");
        DATFile envdat(protein_path + "envelope.dat");
        MDPFile molmdp(mdp_folder + "rerun_mol.mdp");

        if (!envgro.exists() || !envpy.exists() || !envdat.exists() || !molmdp.exists()) {
            MDPFile dummymdp = MDPFile(output + "empty.mdp").create();
            auto[dummytpr] = grompp(dummymdp, moltop, molgro)
                .output(output + "saxs.tpr")
                .warnings(1)
            .run();
            dummymdp.remove();

            auto[itps] = genscatt(dummytpr, molindex)
                .output(protein_path + "scatt.itp")
                .group("Protein")
            .run();

            moltop.include(itps, "");

            // dump the first 50 frames
            auto[traj] = trjconv(molxtc)
                .output(protein_path + "protein.xtc")
                .startframe(50)
            .run();

            // center the trajectories
            auto[cluster] = trjconv(traj)
                .output(protein_path + "cluster.xtc")
                .group("Protein")
                .pbc("cluster")
                .ur("tric")
                .index(molindex)
                .runfile(dummytpr)
            .run();

            auto[centered] = trjconv(cluster)
                .output(protein_path + "centered.xtc")
                .group("Protein")
                .center()
                .boxcenter("tric")
                .pbc("mol")
                .ur("tric")
                .index(molindex)
                .runfile(dummytpr)
            .run();

            // generate the envelope
            auto[goodpbc, envgro, envpy, envdat] = genenv(centered, molindex)
                .output(protein_path)
                .structure(molgro)
                .distance(0.5)
                .group("Protein")
            .run();

            // generate molecule mdp file (depends on envelope output)
            {
                auto _mdp = SAXSMDPCreatorMol();

                Protein protein(options.pdb);
                double qmax = std::stod(_mdp.get(MDPOptions::waxs_endq))/10; // convert to nm^-1
                double dmax = protein.maximum_distance();

                _mdp.add(MDPOptions::waxs_pbcatom = goodpbc);
                _mdp.add(MDPOptions::waxs_nsphere = int(0.2*std::pow((dmax*qmax), 2)));
                _mdp.write(molmdp);
            }
        }

        MDPFile bufmdp(mdp_folder + "rerun_buf.mdp");
        if (!bufmdp.exists()) {
            SAXSMDPCreatorSol().write(bufmdp);
        }

        // generate run files - they can be reused for all parts
        auto[moltpr] = grompp(molmdp, moltop, molgro)
            .output(prod_folder + "mol.tpr")
            .index(molindex)
            .warnings(1)
        .run();

        auto[buftpr] = grompp(bufmdp, buftop, bufgro)
            .output(prod_folder + "buf.tpr")
            .index(bufindex)
            .warnings(2)
        .run();

        // chop the trajectory into separate parts
        std::vector<SAXSOutput> jobs;
        for (unsigned int i = 0; i < 10; ++i) {
            Folder part_folder = prod_folder + "part_" + std::to_string(i) + "/";

            auto [molxtc_i] = trjconv(molxtc)
                .output(part_folder + "mol.xtc")
                .skip_every_n_frame(i+1)
            .run();

            auto [bufxtc_i] = trjconv(bufxtc)
                .output(part_folder + "buf.xtc")
                .skip_every_n_frame(i+1)
            .run();

            auto job = saxsmdrun(moltpr, buftpr)
                .output(part_folder, "prod")
                .jobname(options.name + "_" + std::to_string(i+1))
                .rerun(molxtc_i, bufxtc_i)
                .env_var("GMX_WAXS_FIT_REFFILE", envgro.absolute())
                .env_var("GMX_ENVELOPE_FILE", envdat.absolute())
            .run(options.mainsim, options.jobscript);

            jobs.push_back({std::move(job)});
        }

        return jobs;
    }
}