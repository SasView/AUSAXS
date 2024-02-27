#include <em/ImageStack.h>
#include <settings/All.h>
#include <plots/All.h>
#include <mini/All.h>
#include <fitter/Fit.h>
#include <fitter/LinearFitter.h>
#include <fitter/HydrationFitter.h>
#include <mini/detail/Parameter.h>
#include <em/detail/ExtendedLandscape.h>
#include <em/manager/ProteinManager.h>
#include <data/Molecule.h>
#include <utility/Console.h>
#include <utility/Limit.h>
#include <utility/Utility.h>
#include <constants/Constants.h>
#include <hist/intensity_calculator/DistanceHistogram.h>
#include <hist/intensity_calculator/CompositeDistanceHistogram.h>
#include <settings/EMSettings.h>
#include <settings/HistogramSettings.h>

#include <fstream>

using namespace em;
using namespace fitter;

ImageStack::ImageStack(const io::ExistingFile& file) : ImageStackBase(file) {}

ImageStack::ImageStack(const std::vector<Image>& images) : ImageStackBase(images) {}

ImageStack::~ImageStack() = default;

std::unique_ptr<EMFit> ImageStack::fit(std::unique_ptr<hist::ICompositeDistanceHistogram> h) {
    Limit lim = {from_level(settings::em::alpha_levels.min), from_level(settings::em::alpha_levels.max)};
    mini::Parameter param("cutoff", lim.center(), lim);
    return fit(std::move(h), param);
}

std::unique_ptr<EMFit> ImageStack::fit(std::unique_ptr<hist::ICompositeDistanceHistogram> h, mini::Parameter& param) {
    if (!param.has_bounds()) {return fit(std::move(h));} // ensure parameter bounds are present

    auto limit = Limit(settings::axes::qmin, settings::axes::qmax);
    std::shared_ptr<LinearFitter> fitter = settings::em::hydrate ? std::make_shared<HydrationFitter>(std::move(h), limit) : std::make_shared<LinearFitter>(std::move(h), limit);
    return fit_helper(fitter, param);
}

std::unique_ptr<EMFit> ImageStack::fit(const io::ExistingFile& file) {
    Limit lim = {from_level(settings::em::alpha_levels.min), from_level(settings::em::alpha_levels.max)};
    mini::Parameter param("cutoff", lim.center(), lim);
    return fit(file, param);
}

std::unique_ptr<EMFit> ImageStack::fit(const io::ExistingFile& file, mini::Parameter& param) {
    if (!param.has_bounds()) {return fit(file);} // ensure parameter bounds are present
    std::shared_ptr<LinearFitter> fitter = settings::em::hydrate ? std::make_shared<HydrationFitter>(file) : std::make_shared<LinearFitter>(file);
    return fit_helper(fitter, param);
}

std::unique_ptr<fitter::EMFit> ImageStack::fit_helper(std::shared_ptr<fitter::LinearFitter> fitter) {
    auto p = mini::Parameter();
    return fit_helper(fitter, p);
}

std::unique_ptr<EMFit> ImageStack::fit_helper(std::shared_ptr<LinearFitter> fitter, mini::Parameter& param) {
    //##########################################################//
    //###                       SETUP                        ###//
    //##########################################################//
    settings::molecule::center = false; // do not center the dummy proteins
    if (settings::em::plot_landscapes && settings::em::hydrate) {
        std::static_pointer_cast<HydrationFitter>(fitter)->set_algorithm(mini::type::SCAN);
    }

    update_charge_levels(*param.bounds);
    set_minimum_bounds(param.bounds->min);
    auto f = prepare_function(fitter);
    mini::Landscape evals; // since we'll be using multiple minimizers, we'll need to store the evaluated points manually
    unsigned int dof = fitter->dof();

    //##########################################################//
    //###                DETERMINE LANDSCAPE                 ###//
    //##########################################################//
    mini::LimitedScan minimizer(f, param, settings::fit::max_iterations);
    minimizer.set_limit(5, true);
    SimpleDataset chi2_data;
    {
        auto l = minimizer.landscape(settings::fit::max_iterations);
        evals.append(l);
        chi2_data = l.as_dataset();
    }

    chi2_data.sort_x();
    auto min_abs = chi2_data.find_minimum();

    //##########################################################//
    //### CHECK LANDSCAPE IS OK FOR AVERAGING & INTERPLATION ###//
    //##########################################################//
    chi2_data.limit_y(0, min_abs.y*5);  // focus on the area near the absolute minimum
    if (chi2_data.size_rows() < 10) {       // if we have too few points after imposing the limit, we must sample some more
        Limit bounds;                       // first we determine the bounds of the area we want to sample
        if (chi2_data.size_rows() < 3) {    // if we only have one or two points, sample the area between the neighbouring points
            double s = (param.bounds->max - param.bounds->min)/settings::fit::max_iterations;
            bounds = {min_abs.x - s, min_abs.x + s};
        }
        else { // otherwise just use the new bounds of the limited landscape
            bounds = chi2_data.span_x();
        }

        // prepare a new minimizer with the new bounds
        console::print_warning("Function is varying strongly. Sampling more points around the minimum.");
        mini::LimitedScan mini2(f, mini::Parameter("cutoff", bounds), settings::fit::max_iterations/4);
        {
            auto l = mini2.landscape(settings::fit::max_iterations/2);
            evals.append(l);
            chi2_data = l.as_dataset();
        }
        chi2_data.sort_x();
        min_abs = chi2_data.find_minimum();
        chi2_data.limit_y(0, min_abs.y*5);

        if (chi2_data.size_rows() < 10) {
            throw except::unexpected("ImageStack::fit: Could not sample enough points around the minimum. Function varies too much.");
        }
    }

    //##########################################################//
    //###         AVERAGE & INTERPLATE MORE POINTS           ###//
    //##########################################################//
    Dataset data_avg_int; // cutoff, chi2, mass
    {
        auto ra = chi2_data.rolling_average(7).interpolate(5); // impose a moving average filter
        data_avg_int = Dataset(ra.size(), 3);
        data_avg_int.set_col_names({"cutoff", "chi2", "mass"});
        data_avg_int.col("cutoff") = ra.x();
        data_avg_int.col("chi2") = ra.y();
    }

    double spacing = data_avg_int.x(1)-data_avg_int.x(0); 
    auto minima = data_avg_int.find_minima(static_cast<int>(0.1*data_avg_int.size()), 0.1); // find all minima. they should be fairly spaced out (10% seems reasonable?)

    {   // find the absolute minimum in the smoothed landscape
        auto tmp = data_avg_int.find_minimum(1);
        min_abs = {tmp[0], tmp[1], tmp[2]};
    }

    // prepare the mass axis
    if (settings::em::mass_axis) {
        Dataset mass_data(this->evals.size(), 2);
        for (unsigned int i = 0; i < this->evals.size(); ++i) {
            mass_data[i] = {this->evals[i].cutoff, this->evals[i].mass};
        }
        mass_data.sort_x();
        data_avg_int.col("mass") = mass_data.interpolate(data_avg_int.x()).y();
    } 

    // remove minima that are too far away from the absolute minimum
    {
        std::vector<unsigned int> to_keep;
        for (auto m : minima) {
            if (data_avg_int.y(m) < min_abs.y*2) {to_keep.push_back(m);}
        }
        minima = std::move(to_keep);
    }
    
    // update our parameter since we interpolated more points
    param.guess = min_abs.x;
    param.bounds = Limit(min_abs.x-3*spacing, min_abs.x+3*spacing); // uncertainty is 3*spacing between points

    // save .pdb structures of the other minima
    if (settings::em::save_pdb && 1 < minima.size()) {
        unsigned int enumerate = 0;
        std::string info;
        for (auto m : minima) {
            if (data_avg_int.x(m) == min_abs.x) {continue;}
            auto temp_protein = get_protein_manager()->get_protein(data_avg_int.x(m));
            if (settings::em::hydrate) {
                temp_protein->clear_grid();
                temp_protein->generate_new_hydration();
            }
            temp_protein->save(settings::general::output + "models/model_" + std::to_string(++enumerate) + ".pdb");
            info += "Model " + std::to_string(enumerate) + ": (σ, χ²) = " + std::to_string(to_level(data_avg_int.x(m))) + " " + std::to_string(data_avg_int.y(m)) + "\n";
            if (settings::em::mass_axis) {
                info += "  Estimated mass = " + std::to_string(data_avg_int.col("mass")[m]) + " kDa\n";
            }
        }
        std::ofstream out(settings::general::output + "models/info.txt");
        out << info;
    }

    if (settings::general::supplementary_plots) {
        { // make a nice plot of the landscape within some range of the minimum; this is often nicer to look at than the full landscape due to the reduced y-range
            // plot the minimum in blue
            SimpleDataset p_min, chi2_copy = chi2_data, avg_copy = data_avg_int;
            for (auto m : minima) {p_min.push_back(to_level(data_avg_int.x(m)), data_avg_int.y(m)/dof);}

            // convert cutoff to std levels & normalize chi2
            for (unsigned int i = 0; i < chi2_copy.size(); ++i) {
                chi2_copy.x(i) = to_level(chi2_copy.x(i));
                chi2_copy.y(i) /= dof;
            }
            for (unsigned int i = 0; i < avg_copy.size(); ++i) {
                avg_copy.x(i) = to_level(avg_copy.x(i));
                avg_copy.y(i) /= dof;
            }

            // prepare rest of the plot
            plots::PlotDataset plot(avg_copy, plots::PlotOptions(style::draw::line, {{"color", style::color::red}, {"xlabel", "cutoff [$\\sigma$]"}, {"ylabel", "$\\chi_r^2$"}}));
            plot.plot(chi2_copy, plots::PlotOptions(style::draw::points, {}));
            plot.plot(p_min, plots::PlotOptions(style::draw::points, {{"color", style::color::blue}, {"s", 12}}));
            plot.save(settings::general::output + "chi2_evaluated_points_limited." + settings::plots::format);

            if (settings::em::mass_axis) {
                // create chi2 / mass dataset
                SimpleDataset mass_avg_copy(data_avg_int.col("mass"), data_avg_int.col("chi2")/dof);

                SimpleDataset mass_p_min;
                for (auto m : minima) {mass_p_min.push_back(data_avg_int.col("mass")[m], data_avg_int.col("chi2")[m]/dof);}

                // make the plot
                plots::PlotDataset plot_mass(mass_avg_copy, plots::PlotOptions(style::draw::line, {{"color", style::color::red}, {"xlabel", "mass [kDa]"}, {"ylabel", "$\\chi_r^2$"}}));
                plot_mass.plot(SimpleDataset(data_avg_int.interpolate(chi2_data.x()).col(2), chi2_copy.y()), plots::PlotOptions(style::draw::points, {}));
                plot_mass.plot(mass_p_min, plots::PlotOptions(style::draw::points, {{"color", style::color::blue}, {"s", 12}}));
                plot_mass.save(settings::general::output + "chi2_evaluated_points_limited_mass." + settings::plots::format);
            }

            if (settings::em::hydrate) {
                plots::PlotDataset::quick_plot(
                    get_fitted_water_factors_dataset(),
                    plots::PlotOptions(style::draw::points, {{"xlabel", "Iteration"}, {"ylabel", "Scaling factor"}}),
                    settings::general::output + "water_factors." + settings::plots::format
                );
            }
        }

        { // plot all evaluated points
            auto l = evals.as_dataset();
            l.sort_x();            
            {
                auto l_copy = l;
                for (unsigned int i = 0; i < l_copy.size(); ++i) {
                    l_copy.x(i) = to_level(l_copy.x(i));
                    l_copy.y(i) /= dof;
                }

                plots::PlotDataset::quick_plot(
                    l_copy, 
                    plots::PlotOptions(style::draw::points, {{"xlabel", "cutoff [$\\sigma$]"}, {"ylabel", "$\\chi_r^2$"}}), 
                    settings::general::output + "chi2_evaluated_points_full." + settings::plots::format
                );
            }

            // plot with mass axis
            // if (settings::em::mass_axis) {
            //     plots::PlotDataset::quick_plot(
            //         mass_data,
            //         plots::PlotOptions(style::draw::points, {{"xlabel", "mass [kDa]"}, {"ylabel", "$\\chi_r^2$"}}),
            //         settings::general::output + "chi2_evaluated_points_full_mass." + settings::plots::format
            //     );
            // }
        }
    }

    //##########################################################//
    //###             EXPLORE AREA AROUND MINIMUM            ###//
    //##########################################################//
    // if hydration is enabled, the chi2 will oscillate heavily around the minimum
    // we therefore want to sample the area near the minimum to get an average
    mini::Result res;
    if (settings::em::hydrate) {
        // reset evaluated points
        this->evals.clear();

        // sample the area around the minimum
        mini::MinimumExplorer explorer(f, param, settings::fit::max_iterations);
        res = explorer.minimize();

        // plot evaluated points near the minimum
        if (settings::general::supplementary_plots) {
            SimpleDataset area;
            {
                auto l = explorer.landscape();
                evals.append(l);
                area = l.as_dataset();
                area.y() = area.y()/dof;
            }

            // calculate the mean & standard deviation of the sampled points
            double mu = area.mean();
            double sigma = area.std();

            // plot the starting point in blue
            SimpleDataset p_start;
            p_start.push_back(min_abs.x, min_abs.y/dof);

            // do the actual plotting
            plots::PlotDataset(area, plots::PlotOptions(style::draw::points, {{"xlabel", "cutoff"}, {"ylabel", "$\\chi_r^2$"}}))
                .hline(mu, plots::PlotOptions(style::draw::line, {{"color", style::color::red}}))
                .hline(mu+sigma, plots::PlotOptions(style::draw::line, {{"color", style::color::red}, {"linestyle", "--"}}))
                .hline(mu-sigma, plots::PlotOptions(style::draw::line, {{"color", style::color::red}, {"linestyle", "--"}}))
                .plot(p_start, plots::PlotOptions(style::draw::points, {{"color", style::color::blue}, {"s", 9}}))
            .save(settings::general::output + "chi2_near_minimum." + settings::plots::format);

            // make mass version
            if (settings::em::mass_axis) {
                Dataset mass_cutoff(0, 2);
                // skip the first few points since they are used for calibration
                for (int i = static_cast<int>(this->evals.size() - area.size()); i < static_cast<int>(this->evals.size()); ++i) {
                    mass_cutoff.push_back({this->evals[i].cutoff, this->evals[i].mass});
                }
                mass_cutoff.sort_x();

                // create chi2 / mass dataset
                area.x() = mass_cutoff.y();

                // interpolate start point
                p_start.x(0) = mass_cutoff.interpolate_x(p_start.x(0), 1);

                // make the plot
                plots::PlotDataset(area, plots::PlotOptions(style::draw::points, {{"xlabel", "mass [kDa]"}, {"ylabel", "$\\chi_r^2$"}}))
                    .hline(mu, plots::PlotOptions(style::draw::line, {{"color", style::color::red}}))
                    .hline(mu+sigma, plots::PlotOptions(style::draw::line, {{"color", style::color::red}, {"linestyle", "--"}}))
                    .hline(mu-sigma, plots::PlotOptions(style::draw::line, {{"color", style::color::red}, {"linestyle", "--"}}))
                    .plot(p_start, plots::PlotOptions(style::draw::points, {{"color", style::color::blue}, {"s", 9}}))
                .save(settings::general::output + "chi2_near_minimum_mass." + settings::plots::format);
            }
        }
    } 
    
    // otherwise do a quick fit to ensure we're at the very bottom of the valley
    else {
        mini::Golden golden(f, param);
        res = golden.minimize();
        evals.append(golden.get_evaluated_points());
    }

    // Make 3D landscape plot
    if (settings::em::plot_landscapes && settings::em::hydrate) {
        mini::Landscape l;
        l.evals.reserve(1000);
        for (int i = 0; i < static_cast<int>(this->evals.size()); ++i) {
            for (int j = 0; j < static_cast<int>(this->evals[i].strip.evals.size()); ++j) {
                double x = this->evals[i].cutoff;
                double y = this->evals[i].strip.evals[j].vals.front();
                double z = this->evals[i].strip.evals[j].fval;
                l.evals.push_back(mini::Evaluation({x, y}, z));
            }
        }
        l.add_plot_options({{"xlabel", "cutoff"}, {"ylabel", "c"}, {"zlabel", "$\\chi^2$"}});
        plots::PlotLandscape::quick_plot(l, settings::general::output + "chi2_data." + settings::plots::format);
    }

    // update the fitter with the optimal cutoff, such that the returned fit is actually the best one
    min_abs = evals.as_dataset().find_minimum();
    f({min_abs.x});

    std::unique_ptr<fitter::EMFit> emfit = std::make_unique<EMFit>(*fitter, res, res.fval);
    emfit->evaluated_points = evals;
    emfit->fevals = evals.evals.size();
    emfit->level = to_level(min_abs.x);
    if (settings::em::mass_axis) {emfit->mass = data_avg_int.interpolate_x(min_abs.x, 2);}
    if (settings::em::save_pdb) {
        auto temp_protein = get_protein_manager()->get_protein(min_abs.x);
        if (settings::em::hydrate) {
            temp_protein->clear_grid();
            temp_protein->generate_new_hydration();
        }
        temp_protein->save(settings::general::output + "model.pdb");}
    return emfit;
}

std::function<double(std::vector<double>)> ImageStack::prepare_function(std::shared_ptr<LinearFitter> fitter) {
    // convert the calculated intensities to absolute scale
    // utility::print_warning("Warning in ImageStack::prepare_function: Not using absolute scale.");
    // auto protein = phm->get_protein(1);
    // double c = settings::em::concentration;                                // concentration
    // double m = protein->get_absolute_mass()*constants::unit::mg;          // mass
    // double DrhoV2 = std::pow(protein->get_relative_charge(), 2);          // charge
    // double re2 = pow(constants::radius::electron*constants::unit::cm, 2); // squared scattering length
    // double I0 = DrhoV2*re2*c/m;
    // fitter.normalize_intensity(I0);

    // fitter is captured by value to guarantee its lifetime will be the same as the lambda
    // 'this' is ok since prepare_function is private and thus only used within the class itself
    std::function<double(std::vector<double>)> chi2 = [this, fitter] (const std::vector<double>& params) {
        static unsigned int counter = 0;
        static double last_c = 5;

        std::shared_ptr<Fit> fit;
        auto p = get_protein_manager()->get_protein(params[0]);
        if (settings::em::hydrate) {
            p->clear_grid();                // clear grid from previous iteration
            p->generate_new_hydration();    // generate a new hydration layer

            // pointer cast is ok since the type should always be HydrationFitter when hydration is enabled
            std::static_pointer_cast<HydrationFitter>(fitter)->set_guess(mini::Parameter{"c", last_c, {0, 200}});
            fitter->set_scattering_hist(p->get_histogram());

            auto mass = p->get_volume_grid()*constants::SI::volume::A3                                      // essentially free to calculate, so we always do it
                *constants::mass::density::protein                                                          
                /constants::SI::mass::u/1e3;                                                                // conversion factor to get mass in kDa
            fit = fitter->fit();                                                                            // do the fit
            water_factors.push_back(fit->get_parameter("c"));                                               // record c value
            last_c = fit->get_parameter("c").value;                                                         // update c for next iteration
            if (last_c < 0.05) {last_c = 0;}                                                                // ensure that we consider the possibility of no hydration for small c
            evals.push_back(detail::ExtendedLandscape(params[0], mass, std::move(fit->evaluated_points)));  // record evaluated points
        } else {
            p->clear_grid();                                                                                // clear grid from previous iteration
            auto mass = p->get_volume_grid()*constants::SI::volume::A3                                      // essentially free to calculate, so we always do it
                *constants::mass::density::protein                                                          
                /constants::SI::mass::u/1e3;                                                                // conversion factor to get mass in kDa
            fitter->set_scattering_hist(p->get_histogram());
            fit = fitter->fit();
            evals.push_back(detail::ExtendedLandscape(params[0], mass, std::move(fit->evaluated_points)));  // record evaluated points
        }

        double val = fit->fval;
        progress.notify(counter++);
        if (settings::fit::verbose) {
            std::cout << "Step " << utility::print_element(counter, 4) << ": Evaluated cutoff value " << utility::print_element(params[0], 8) << " with chi2 " << utility::print_element(val, 8) << std::flush << "\r";
        }
        return val;
    }; 
    return chi2;
}


mini::Landscape ImageStack::cutoff_scan(const Axis& points, const io::ExistingFile& file) {
    std::shared_ptr<LinearFitter> fitter = settings::em::hydrate ? std::make_shared<HydrationFitter>(file) : std::make_shared<LinearFitter>(file);
    return cutoff_scan_helper(points, fitter);
}

mini::Landscape ImageStack::cutoff_scan(unsigned int points, const io::ExistingFile& file) {
    Axis axis(from_level(settings::em::alpha_levels.min), from_level(settings::em::alpha_levels.max), points);
    return cutoff_scan(axis, file);
}

mini::Landscape ImageStack::cutoff_scan(const Axis& points, std::unique_ptr<hist::ICompositeDistanceHistogram> h) {
    auto limit = Limit(settings::axes::qmin, settings::axes::qmax);
    std::shared_ptr<LinearFitter> fitter = settings::em::hydrate ? std::make_shared<HydrationFitter>(std::move(h), limit) : std::make_shared<LinearFitter>(std::move(h), limit);
    return cutoff_scan_helper(points, fitter);
}

mini::Landscape ImageStack::cutoff_scan(unsigned int points, std::unique_ptr<hist::ICompositeDistanceHistogram> h) {
    Axis axis(from_level(settings::em::alpha_levels.min), from_level(settings::em::alpha_levels.max), points);
    return cutoff_scan(axis, std::move(h));
}

std::pair<EMFit, mini::Landscape> ImageStack::cutoff_scan_fit(unsigned int points, std::unique_ptr<hist::ICompositeDistanceHistogram> h) {
    Axis axis(from_level(settings::em::alpha_levels.min), from_level(settings::em::alpha_levels.max), points);
    return cutoff_scan_fit(axis, std::move(h));
}

std::pair<EMFit, mini::Landscape> ImageStack::cutoff_scan_fit(const Axis& points, const io::ExistingFile& file) {
    std::shared_ptr<LinearFitter> fitter = settings::em::hydrate ? std::make_shared<HydrationFitter>(file) : std::make_shared<LinearFitter>(file);    
    return cutoff_scan_fit_helper(points, fitter);
}

std::pair<EMFit, mini::Landscape> ImageStack::cutoff_scan_fit(unsigned int points, const io::ExistingFile& file) {
    Axis axis(from_level(settings::em::alpha_levels.min), from_level(settings::em::alpha_levels.max), points);
    std::shared_ptr<LinearFitter> fitter = settings::em::hydrate ? std::make_shared<HydrationFitter>(file) : std::make_shared<LinearFitter>(file);    
    return cutoff_scan_fit_helper(axis, fitter);
}

mini::Landscape ImageStack::cutoff_scan_helper(const Axis& points, std::shared_ptr<LinearFitter> fitter) {
    update_charge_levels(points.limits());
    set_minimum_bounds(points.min);
    auto func = prepare_function(fitter);

    mini::Golden minimizer(func, mini::Parameter{"cutoff", points.limits()});
    return minimizer.landscape(points.bins);
}

std::pair<EMFit, mini::Landscape> ImageStack::cutoff_scan_fit(const Axis& points, std::unique_ptr<hist::ICompositeDistanceHistogram> h) {
    auto limit = Limit(settings::axes::qmin, settings::axes::qmax);
    std::shared_ptr<LinearFitter> fitter = settings::em::hydrate ? std::make_shared<HydrationFitter>(std::move(h), limit) : std::make_shared<LinearFitter>(std::move(h), limit);
    return cutoff_scan_fit_helper(points, fitter);
}

std::pair<EMFit, mini::Landscape> ImageStack::cutoff_scan_fit_helper(const Axis& points, std::shared_ptr<LinearFitter> fitter) {
    update_charge_levels(points.limits());
    set_minimum_bounds(points.min);
    auto func = prepare_function(fitter);

    // cutoff scan
    mini::Golden minimizer(func, mini::Parameter{"cutoff", points.limits()});
    mini::Landscape landscape = minimizer.landscape(points.bins);

    // fit
    double l = from_level(1);
    Limit limit(settings::em::alpha_levels.min*l, settings::em::alpha_levels.max*l);
    minimizer.clear_parameters();
    minimizer.add_parameter({"cutoff", limit.center(), limit});
    auto res = minimizer.minimize();

    EMFit emfit(*fitter, res, res.fval);
    emfit.evaluated_points = minimizer.get_evaluated_points();

    return {emfit, landscape};
}

const std::vector<mini::FittedParameter>& ImageStack::get_fitted_water_factors() const {
    return water_factors;
}

SimpleDataset ImageStack::get_fitted_water_factors_dataset() const {
    std::vector<double> x(water_factors.size()), y(water_factors.size());
    for (unsigned int i = 0; i < water_factors.size(); i++) {
        x[i] = i;
        y[i] = water_factors[i].value;
    }
    return SimpleDataset(x, y, "Iteration", "Scaling factor");
}

void ImageStack::update_charge_levels(const Limit& limit) const noexcept {
    std::vector<double> levels;
    for (unsigned int i = 0; i < settings::em::charge_levels; i++) {
        levels.push_back(limit.min + i*limit.span()/settings::em::charge_levels);
    }
    get_protein_manager()->set_charge_levels(levels);
}