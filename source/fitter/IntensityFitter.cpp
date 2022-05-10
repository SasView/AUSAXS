#include <fitter/Fit.h>
#include <fitter/IntensityFitter.h>
#include <math/SimpleLeastSquares.h>
#include <math/CubicSpline.h>
#include <histogram/ScatteringHistogram.h>
#include <utility/Exceptions.h>

#include <Math/Minimizer.h>
#include <Math/Factory.h>
#include <Math/Functor.h>

using std::string, std::vector, std::shared_ptr, std::unique_ptr;

shared_ptr<Fit> IntensityFitter::fit() {
    ROOT::Math::Minimizer* minimizer = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");
    auto f = std::bind(&IntensityFitter::chi2, this, std::placeholders::_1);
    ROOT::Math::Functor functor(f, 1); // declare the function to be minimized and its number of parameters
    minimizer->SetFunction(functor);
    minimizer->SetPrintLevel(0);
    minimizer->SetLimitedVariable(0, "c", 5, 1e-4, 0, 100); // scaling factor
    minimizer->Minimize();
    const double* res = minimizer->X();
    const double* err = minimizer->Errors();

    // apply c
    h.apply_water_scaling_factor(res[0]);
    vector<double> ym = h.calc_debye_scattering_intensity().get("I");
    vector<double> Im = splice(ym);

    // we want to fit a*Im + b to Io
    Dataset fit_data(Im, data.y, data.yerr);
    if (I0 > 0) {fit_data.normalize(I0);}

    SimpleLeastSquares fitter(fit_data);
    std::shared_ptr<Fit> ab_fit = fitter.fit();

    // update fitter object
    bool converged = !minimizer->Status();
    std::map<string, double> pars = {{"c", res[0]}, {"a", ab_fit->params["a"]}, {"b", ab_fit->params["b"]}};
    std::map<string, double> errs = {{"c", err[0]}, {"a", ab_fit->errors["a"]}, {"b", ab_fit->errors["b"]}};
    double funcalls = minimizer->NCalls();
    fitted = std::make_shared<Fit>(pars, errs, chi2(res), data.size()-2, funcalls, converged);

    minimizer->SetPrintLevel(3);
    minimizer->PrintResults();
    return fitted;
}

Fit::Plots IntensityFitter::plot() {
    if (fitted == nullptr) {throw except::bad_order("Error in IntensityFitter::plot: Cannot plot before a fit has been made!");}

    double a = fitted->params["a"];
    double b = fitted->params["b"];
    double c = fitted->params["c"];

    h.apply_water_scaling_factor(c);
    vector<double> ym = h.calc_debye_scattering_intensity().get("I");
    vector<double> Im = splice(ym);

    // calculate the scaled I model values
    vector<double> I_scaled(data.size()); // spliced data
    vector<double> ym_scaled(ym.size()); // original scaled data
    std::transform(Im.begin(), Im.end(), I_scaled.begin(), [&a, &b] (double I) {return I*a+b;});
    std::transform(ym.begin(), ym.end(), ym_scaled.begin(), [&a, &b] (double I) {return I*a+b;});

    // prepare the TGraphs
    vector<double> xerr(data.size(), 0);
    Fit::Plots graphs;
    graphs.intensity_interpolated = SAXSDataset(data.x, I_scaled);
    graphs.intensity = SAXSDataset(h.q, ym_scaled);
    graphs.data = SAXSDataset(data.x, data.y, xerr, data.yerr);
    return graphs;
}

Dataset IntensityFitter::plot_residuals() {
    if (fitted == nullptr) {throw except::bad_order("Error in IntensityFitter::plot_residuals: Cannot plot before a fit has been made!");}
 
    double a = fitted->params["a"];
    double b = fitted->params["b"];
    double c = fitted->params["c"];

    h.apply_water_scaling_factor(c);
    vector<double> ym = h.calc_debye_scattering_intensity().get("I");
    vector<double> Im = splice(ym);

    // calculate the residuals
    vector<double> residuals(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        residuals[i] = ((data.y[i] - (a*Im[i]+b))/data.yerr[i]);
    }

    // prepare the TGraph
    vector<double> xerr(data.size(), 0);
    return Dataset(data.x, residuals, xerr, data.yerr);
}

double IntensityFitter::chi2(const double* params) {
    double c = params[0];

    // apply c
    h.apply_water_scaling_factor(c);
    vector<double> ym = h.calc_debye_scattering_intensity().get("I");
    vector<double> Im = splice(ym);

    // we want to fit a*Im + b to Io
    Dataset fit_data(Im, data.y, data.yerr);
    if (I0 > 0) {fit_data.normalize(I0);}

    SimpleLeastSquares fitter(fit_data);
    auto[a, b] = fitter.fit_params_only();

    // calculate chi2
    std::cout << "FITDATA: " << fit_data.y[0] << ", DATA: " << data.y[0] << std::endl;
    double chi = 0;
    for (size_t i = 0; i < data.size(); i++) {
        double v = (data.y[i] - (a*Im[i]+b))/data.yerr[i];
        chi += v*v;
    }
    return chi;
}

double IntensityFitter::get_intercept() {
    if (fitted == nullptr) {throw except::bad_order("Error in IntensityFitter::get_intercept: Cannot determine model intercept before a fit has been made!");}
 
    double a = fitted->params["a"];
    double b = fitted->params["b"];
    double c = fitted->params["c"];

    h.apply_water_scaling_factor(c);
    vector<double> ym = h.calc_debye_scattering_intensity().get("I");
    CubicSpline s(h.q, ym);
    return a*s.spline(0) + b;
}

SAXSDataset IntensityFitter::get_model_dataset() {
    if (fitted == nullptr) {throw except::bad_order("Error in IntensityFitter::get_model_dataset: Cannot determine model intercept before a fit has been made!");}
 
    double a = fitted->params["a"];
    double b = fitted->params["b"];
    double c = fitted->params["c"];

    h.apply_water_scaling_factor(c);
    vector<double> ym = h.calc_debye_scattering_intensity().get("I");
    vector<double> Im = splice(ym);
    std::transform(Im.begin(), Im.end(), Im.begin(), [&a, &b] (double I) {return I*a+b;});

    return SAXSDataset(data.x, Im, "q", "I"); 
}

SAXSDataset IntensityFitter::get_model_dataset(const vector<double>& q) {
    if (fitted == nullptr) {throw except::bad_order("Error in IntensityFitter::get_model_dataset: Cannot determine model intercept before a fit has been made!");}
 
    double a = fitted->params["a"];
    double b = fitted->params["b"];
    double c = fitted->params["c"];

    h.apply_water_scaling_factor(c);
    SAXSDataset model = h.calc_debye_scattering_intensity(q);
    std::transform(model.y.begin(), model.y.end(), model.y.begin(), [&a, &b] (double I) {return I*a+b;});
    return model;
}

SAXSDataset IntensityFitter::get_dataset() const {
    return data;
}