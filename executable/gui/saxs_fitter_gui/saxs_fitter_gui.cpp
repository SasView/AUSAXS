#include <elements.hpp>
#include <nfd.hpp>
#include <include.h>

#include <io/File.h>
#include <constants/Constants.h>
#include <data/Molecule.h>
#include <dataset/SimpleDataset.h>
#include <hist/intensity_calculator/ICompositeDistanceHistogram.h>
#include <fitter/HydrationFitter.h>
#include <fitter/ExcludedVolumeFitter.h>
#include <fitter/FitReporter.h>
#include <fitter/Fit.h>
#include <settings/All.h>
#include <plots/All.h>

#include <bitset>
#include <thread>
#include <filesystem>
#include <string_view>

namespace gui = cycfi::elements;

namespace settings {
	bool fit_excluded_volume = false;
}

auto plot_names = std::vector<std::pair<std::string, std::string>>{
	{"log", "single-logarithmic plot"},
	{"loglog", "double-logarithmic plot"},
	{"p(r)", "distance histogram"},
	{"profiles", "partial profiles"}
};

auto make_start_button(gui::view& view) {
	static auto start_button = gui::button("start");
	start_button->set_body_color(ColorManager::get_color_success());

	auto start_button_layout = gui::margin(
		{10, 100, 10, 100},
		gui::align_center_middle(
			gui::hsize(
				200,
				link(start_button)
			)
		)
	);

	static auto deck = gui::deck(
		start_button_layout,
		start_button_layout
	);

	static std::thread worker;
	start_button.on_click = [&view] (bool) {
		// ensure the worker is ready to be assigned a job
		if (worker.joinable()) {
			worker.join();
		}

		if (!setup::saxs_dataset || !io::File(settings::pdb_file).exists()) {
			std::cout << "no saxs data or pdb file was provided" << std::endl;
			start_button->set_body_color(ColorManager::get_color_fail());
			start_button->set_text("missing input");
			view.refresh(start_button);
			worker = std::thread([&view] () {
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				start_button->set_body_color(ColorManager::get_color_success());
				start_button->set_text("start");
				view.refresh(start_button);
			});
			return;
		}

		start_button->set_body_color(ColorManager::get_color_accent());
		start_button->set_text("working...");

		// use a worker thread to avoid locking the gui
		worker = std::thread([&view] () {
			bool fit_excluded_volume = settings::fit_excluded_volume;
			setup::pdb = std::make_unique<data::Molecule>(settings::pdb_file);

			std::shared_ptr<fitter::HydrationFitter> fitter;
			if (fit_excluded_volume) {fitter = std::make_shared<fitter::ExcludedVolumeFitter>(settings::saxs_file, setup::pdb->get_histogram());}
			else {fitter = std::make_shared<fitter::HydrationFitter>(settings::saxs_file, setup::pdb->get_histogram());}
			std::shared_ptr<fitter::Fit> result = fitter->fit();

			fitter::FitReporter::report(result.get());
			fitter::FitReporter::save(result.get(), settings::general::output + "report.txt");

			plots::PlotDistance::quick_plot(fitter->get_scattering_hist(), settings::general::output + "p(r)." + settings::plots::format);
			plots::PlotProfiles::quick_plot(fitter->get_scattering_hist(), settings::general::output + "profiles." + settings::plots::format);

			fitter->get_model_dataset().save(settings::general::output + "fit.fit");
			fitter->get_dataset().save(settings::general::output + io::File(settings::saxs_file).stem() + ".scat");

			setup::pdb->save(settings::general::output + "model.pdb");
			perform_plot(settings::general::output);

			auto make_image_pane = [] (const io::File& path) {
				return gui::image(std::filesystem::current_path().string() + "/" + path.path().c_str(), 0.13);
			};

			auto main_pane = gui::vnotebook(
				view,
				gui::deck(
					make_image_pane(settings::general::output + plot_names[0].first + ".png"),
					make_image_pane(settings::general::output + plot_names[1].first + ".png"),
					make_image_pane(settings::general::output + plot_names[2].first + ".png"),
					make_image_pane(settings::general::output + plot_names[3].first + ".png")
				),
				gui::tab(plot_names[0].second),
				gui::tab(plot_names[1].second),
				gui::tab(plot_names[2].second),
				gui::tab(plot_names[3].second)
			);

			auto image_viewer_layout = 	gui::margin(
				{10, 10, 10, 10},
				gui::vtile(
					main_pane,
					gui::margin_top(
						10,
						gui::align_center_middle(
							gui::hsize(
								200,
								link(start_button)
							)
						)
					)
				)
			);

			start_button->set_body_color(ColorManager::get_color_success());
			start_button->set_text("start");

			deck[1] = gui::share(image_viewer_layout);
			deck.select(1);
			view.refresh();
		});
	};

	return link(deck);
}

auto io_menu(gui::view& view) {
	static auto saxs_box_bg = ColorManager::new_background_color();
	static auto pdb_box_bg = ColorManager::new_background_color();
	static auto output_box_bg = ColorManager::new_background_color();

	static auto saxs_box = gui::input_box("saxs path");
	static auto pdb_box = gui::input_box("pdb path");
	static auto output_box = gui::input_box("output path");
	ColorManager::manage_input_box(saxs_box.second);
	ColorManager::manage_input_box(pdb_box.second);
	ColorManager::manage_input_box(output_box.second);

	static bool default_output = true;
	output_box.second->set_text("output/saxs_fitter");
	auto ref = output_box.second;
	static bool pdb_ok = false, saxs_ok = false;

	pdb_box.second->on_text = [&view] (std::string_view text) {
		if (text.size() == 1) {
			pdb_box_bg.get()._color = ColorManager::get_color_accent();
		} else if (text.empty()) {
			pdb_box_bg.get()._color = ColorManager::get_color_background();
		}

		if (pdb_ok) {
			pdb_ok = false;
			pdb_box_bg.get() = ColorManager::get_color_accent();
		}

		static unsigned int last_size = 0;
		auto fill = autocomplete(text, last_size, [] (const io::File& p) {return constants::filetypes::structure.validate(p);});
		if (!fill.first.empty()) {pdb_box.second->set_text(fill.first);}
		if (fill.second) {pdb_box.second->on_enter(fill.first);}
	};

	pdb_box.second->on_enter = [&view] (std::string_view text) {
		io::File file = io::File(std::string(text));
		if (!constants::filetypes::structure.validate(file)) {
			std::cout << "invalid pdb file " << file.path() << std::endl;
			pdb_box_bg.get() = ColorManager::get_color_fail();
			pdb_ok = false;
			return;
		}

		// check if we can use a relative path instead of absolute
		if (auto curpath = std::filesystem::current_path().string(); file.path().find(curpath) != std::string::npos) {
			file = std::filesystem::relative(file.path(), curpath).string();
		}
		pdb_box.second->set_text(file.path());

		settings::pdb_file = file.path();
		std::cout << "pdb file was set to " << settings::pdb_file << std::endl;
		pdb_box_bg.get() = ColorManager::get_color_success();
		pdb_ok = true;

		if (!saxs_ok) {
			if (20 < std::distance(std::filesystem::directory_iterator(file.directory().path()), std::filesystem::directory_iterator{})) {return;}
			for (auto& p : std::filesystem::directory_iterator(file.directory().path())) {
				io::File tmp(p.path().string());
				if (constants::filetypes::saxs_data.validate(tmp)) {
					settings::saxs_file = tmp.path();
					saxs_box.second->set_text(tmp.path());
					saxs_box.second->on_enter(tmp.path());
				}
			}
		}

		if (saxs_ok && default_output) {
			std::string path = "output/saxs_fitter/" + io::File(settings::pdb_file).stem() + "/" + io::File(settings::saxs_file).stem();
			output_box.second->set_text(path);
			output_box.second->on_enter(path);
		}
	};

	saxs_box.second->on_text = [&view] (std::string_view text) {
		if (text.size() == 1) {
			saxs_box_bg.get() = ColorManager::get_color_accent();
		} else if (text.empty()) {
			saxs_box_bg.get() = ColorManager::get_color_background();
		}

		if (saxs_ok) {
			saxs_ok = false;
			saxs_box_bg.get() = ColorManager::get_color_accent();
		}

		static unsigned int last_size = 0;
		auto fill = autocomplete(text, last_size, [] (const io::File& p) {return constants::filetypes::saxs_data.validate(p);});
		if (!fill.first.empty()) {pdb_box.second->set_text(fill.first);}
		if (fill.second) {saxs_box.second->on_enter(fill.first);}
	};

	saxs_box.second->on_enter = [&view] (std::string_view text) {
		io::File file = io::File(std::string(text));
		if (!constants::filetypes::saxs_data.validate(file)) {
			std::cout << "invalid saxs file " << file.path() << std::endl;
			saxs_box_bg.get() = ColorManager::get_color_fail();
			saxs_ok = false;
			setup::saxs_dataset = nullptr;
			return;
		}

		// check if we can use a relative path instead of absolute
		if (auto curpath = std::filesystem::current_path().string(); file.path().find(curpath) != std::string::npos) {
			file = std::filesystem::relative(file.path(), curpath).string();
		}
		saxs_box.second->set_text(file.path());

		std::cout << "saxs file was set to " << settings::saxs_file << std::endl;
		settings::saxs_file = file.path();
		saxs_box_bg.get() = ColorManager::get_color_success();
		setup::saxs_dataset = std::make_unique<SimpleDataset>(settings::saxs_file);
		saxs_ok = true;

		if (pdb_ok) {
		 	if (default_output || output_box.second->get_text().empty()) {
				std::string path = "output/saxs_fitter/" + io::File(settings::pdb_file).stem() + "/" + io::File(settings::saxs_file).stem();
				output_box.second->set_text(path);
				output_box.second->on_enter(path);
			}
		}
	};

	output_box.second->on_text = [] (std::string_view text) {
		if (text.size() == 1) {
			output_box_bg.get() = ColorManager::get_color_accent();
		} else if (text.empty()) {
			output_box_bg.get() = ColorManager::get_color_background();
		}
		default_output = false;
	};

	output_box.second->on_enter = [&view] (std::string_view text) {
		settings::general::output = text;
		if (settings::general::output.back() != '/') {
			settings::general::output += "/";
			view.refresh(output_box.first);
		}
		std::cout << "output path was set to " << settings::general::output << std::endl;
	};

	auto map_box_field = make_file_dialog_button(pdb_box, pdb_box_bg, {"PDB file", "pdb"});
	auto saxs_box_field = make_file_dialog_button(saxs_box, saxs_box_bg, {"SAXS data", "dat,scat"});
	auto output_box_field = make_folder_dialog_button(output_box, output_box_bg);

	return gui::htile(
		gui::margin(
			{50, 10, 50, 10},
			gui::hsize(
				300,
				map_box_field
			)
		),
		gui::margin(
			{50, 10, 50, 10},
			gui::hsize(
				300,
				saxs_box_field
			)
		),
		gui::margin(
			{50, 10, 50, 10},
			gui::hsize(
				300,
				output_box_field
			)
		)
	);
}

auto selection_menu_settings(gui::view&) {
	// we use a deck composite to avoid circular dependencies
	static auto deck = gui::deck_composite();

	std::vector<std::pair<std::string, settings::grid::PlacementStrategy>> options1 {
		{"1. Radial placement", settings::grid::PlacementStrategy::RadialStrategy},
		{"2. Axial placement", settings::grid::PlacementStrategy::AxesStrategy},
		{"3. No hydration", settings::grid::PlacementStrategy::NoStrategy}
	};
	static auto hydration_model = gui::selection_menu(
		[options1] (std::string_view selection) {
			for (auto& option : options1) {
				if (option.first == selection) {
					settings::grid::placement_strategy = option.second;
				}
			}
		}, 
		{
			options1[0].first,
			options1[1].first,
			options1[2].first
		}
	);

	std::vector<std::pair<std::string, settings::hist::HistogramManagerChoice>> options2 {
		{"1. Default form-factor", settings::hist::HistogramManagerChoice::HistogramManagerMT},
		{"2. Unique form-factors", settings::hist::HistogramManagerChoice::HistogramManagerMTFFAvg},
		{"3. Atomic volumes", settings::hist::HistogramManagerChoice::HistogramManagerMTFFExplicit},
		{"4. Occupied grid cells", settings::hist::HistogramManagerChoice::HistogramManagerMTFFGrid}
	};

	static auto excluded_volume_model = gui::selection_menu(
		[options2] (std::string_view selection) {
			for (auto& option : options2) {
				if (option.first == selection) {
					settings::hist::histogram_manager = option.second;
				}
			}
			switch (settings::hist::histogram_manager) {
				case settings::hist::HistogramManagerChoice::HistogramManager:
				case settings::hist::HistogramManagerChoice::HistogramManagerMT:
				case settings::hist::HistogramManagerChoice::PartialHistogramManager:
				case settings::hist::HistogramManagerChoice::PartialHistogramManagerMT:
					settings::molecule::use_effective_charge = true;
					settings::fit_excluded_volume = false;
					deck.select(0);
					break;
				default:
					settings::molecule::use_effective_charge = false;
					deck.select(1);
					break;
			}
		}, 
		{
			options2[0].first,
			options2[1].first,
			options2[2].first,
			options2[3].first
		}
	);

	static auto fit_excluded_volume_button = gui::check_box("fit excluded volume");
	fit_excluded_volume_button.on_click = [] (bool value) {
		settings::fit_excluded_volume = value;
	};

	auto exv_fit_support_layout = gui::margin_left_right(
		{10, 10},
		gui::htile(
			gui::vtile(
				gui::margin_bottom(
					10,
					gui::label("hydration model")
						.font_color(ColorManager::get_text_color())
						.font_size(18)
				),
				link(hydration_model.first)
			),
			gui::hspace(50),
			gui::vtile(
				gui::margin_bottom(
					10,
					gui::label("excluded volume model")
						.font_color(ColorManager::get_text_color())
						.font_size(18)
				),
				link(excluded_volume_model.first)
			),
			gui::hspace(50),
			gui::align_center_middle(
				gui::fixed_size(
					{200, 100},
					link(fit_excluded_volume_button)
				)
			)
		)
	);

	auto no_exv_fit_support_layout = gui::margin_left_right(
		{10, 10},
		gui::htile(
			gui::vtile(
				gui::margin_bottom(
					10,
					gui::label("hydration model")
						.font_color(ColorManager::get_text_color())
						.font_size(18)
				),
				link(hydration_model.first)
			),
			gui::hspace(50),
			gui::vtile(
				gui::margin_bottom(
					10,
					gui::label("excluded volume model")
						.font_color(ColorManager::get_text_color())
						.font_size(18)
				),
				link(excluded_volume_model.first)
			)
		)
	);

	deck.push_back(share(no_exv_fit_support_layout));
	deck.push_back(share(exv_fit_support_layout));
	deck.select(0);

	return link(deck);
}

// toggle light/dark mode
auto toggle_mode_button(gui::view& view) {
	static auto button = gui::button("light mode");
	button.on_click = [&view] (bool) {
		ColorManager::switch_mode();
		button->set_text(ColorManager::dark_mode ? "light mode" : "dark mode");
		view.refresh();
	};
	return link(button);
}

#include <logo.h>
#include <resources.h>
int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
	gui::app app(argc, argv, "AUSAXS intensity fitter", "com.cycfi.ausaxs-intensity-fitter");
	gui::window win(app.name(), std::bitset<4>{"1111"}.to_ulong(), {50, 50, 1024+50, 768+50});
	win.on_close = [&app]() {app.stop();};

	resources::generate_resource_file();
	auto logo_path = resources::generate_logo_file();

	gui::view view(win);
	auto header = gui::layer(
		gui::align_center_top(
			gui::label("Intensity fitter")
				.font_size(50)
				.font_color(ColorManager::get_text_color())
		),
		gui::align_right_top(
			gui::margin(
				{0, 10, 10, 10},
				gui::fixed_size(
					{ 50, 50 },
					toggle_mode_button(view)
				)
			)
		)
	);
	auto content = gui::margin(
		{10, 10, 10, 10},
		gui::vtile(
			io_menu(view),
			gui::htile(
				q_slider(view),
				selection_menu_settings(view)
			),
			gui::align_center_middle(
				make_start_button(view)
			)
		)
	);
	auto footer = gui::margin(
		{10, 10, 10, 10},
		gui::hgrid(
			{0.33, 0.66, 1},
			gui::align_left_bottom(
				gui::label(std::string(constants::version)).font_color(ColorManager::get_text_color())
			),
			gui::align_center_bottom(
				gui::label("Kristian Lytje & Jan Skov Pedersen").font_color(ColorManager::get_text_color())
			),
			gui::align_right_bottom(
				gui::scale_element(0.15, gui::image(logo_path.absolute_path().c_str()))
			)
		)
	);

	view.content(
		gui::vtile(
			header,
			content,
			footer
		),
		link(background)
	);

	app.run();
	return 0;
}
