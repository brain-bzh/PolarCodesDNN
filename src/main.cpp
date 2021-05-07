#include <functional>
#include <exception>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>

#include <aff3ct.hpp>
#include "aff3ct_init.hpp"
#include "gen_fb_list.hpp"

using namespace aff3ct;


int main(int argc, char** argv)
{
	// get the AFF3CT version
	const std::string v = "v" + std::to_string(tools::version_major()) + "." +
	                            std::to_string(tools::version_minor()) + "." +
	                            std::to_string(tools::version_release());
	std::cout << "#--------------------------------------------------"  << std::endl;
	std::cout << "# This program uses the AFF3CT library (" << v << ")" << std::endl;
	std::cout << "#--------------------------------------------------"  << std::endl;
	std::cout << "#"                                                    << std::endl;

	params  p; init_params (argc, argv, p); // create and initialize the parameters from the command line with factories
	modules m; init_modules(p, m         ); // create and initialize the modules

	// sockets binding (connect the sockets of the tasks = fill the input sockets with the output sockets)
	using namespace module;
	(*m.crc    )[crc::sck::build       ::U_K1].bind((*m.source )[src::sck::generate   ::U_K ], false 	);
	(*m.encoder)[enc::sck::encode      ::U_K ].bind((*m.crc    )[crc::sck::build      ::U_K2], false 	);
	(*m.modem  )[mdm::sck::modulate    ::X_N1].bind((*m.encoder)[enc::sck::encode     ::X_N ], false 	);
	(*m.channel)[chn::sck::add_noise   ::X_N ].bind((*m.modem  )[mdm::sck::modulate   ::X_N2], false 	);
	(*m.modem  )[mdm::sck::demodulate  ::Y_N1].bind((*m.channel)[chn::sck::add_noise  ::Y_N ], false 	);
	(*m.decoder)[dec::sck::decode_siho ::Y_N ].bind((*m.modem  )[mdm::sck::demodulate ::Y_N2], false 	);
	(*m.crc    )[crc::sck::extract     ::V_K1].bind((*m.decoder)[dec::sck::decode_siho::V_K ], false 	);
	(*m.monitor)[mnt::sck::check_errors::U   ].bind((*m.source )[src::sck::generate   ::U_K ], false 	);
	(*m.monitor)[mnt::sck::check_errors::V   ].bind((*m.crc    )[crc::sck::extract    ::V_K2], false 	);

	utils   u; init_utils  (p, m, u      ); // create and initialize the utils

	std::ofstream ofs;
  	ofs.open ("fer" + std::to_string(p.gad->seed) + ".txt", std::ofstream::out | std::ofstream::trunc);
	if (!ofs.is_open())
		throw runtime_error(__FILE__, __LINE__, __func__, std::string("Unable to open fer.txt."));

	// display the legend in the terminal
	if (p.gad->enable_terminal)
		u.terminal->legend();

	for (auto &m : u.chain->get_modules<tools::Interface_get_set_noise>())
		m->set_noise(*u.noise);

	for (auto &m : u.chain->get_modules<tools::Interface_notify_noise_update>())
		u.noise->record_callback_update([m](){ m->notify_noise_update(); });

	// set different seeds in the modules that uses PRNG
	std::mt19937 prng;
	for (auto &m : u.chain->get_modules<tools::Interface_set_seed>())
		m->set_seed(prng());

	// compute the current sigma for the channel noise
	const auto ebn0 = p.gad->ebn0;
	const auto esn0  = tools::ebn0_to_esn0 (ebn0, p.R, p.modem->bps);
	const auto sigma = tools::esn0_to_sigma(esn0, p.modem->cpm_upf );

	u.noise->set_values(sigma, ebn0, esn0);

	if (p.gad->fb_file.empty())
	{
		if (p.gad->fb_ebn0 != p.gad->ebn0)
		{
			const auto fb_esn0  = tools::ebn0_to_esn0 (p.gad->fb_ebn0, p.R, p.modem->bps);
			const auto fb_sigma = tools::esn0_to_sigma(esn0, p.modem->cpm_upf );
			tools::Sigma<> fb_noise(fb_sigma,p.gad->fb_ebn0, fb_esn0);
			if (p.gad->polar_rm)
				gen_fb_polar_rm(p, fb_noise);
			else
				gen_fb_list_shuffle(p, fb_noise);
		}
		else
			if (p.gad->polar_rm)
				gen_fb_polar_rm    (p, *u.noise);
			else
				gen_fb_list_shuffle(p, *u.noise);

	}
	else
		gen_fb_list_from_file(p);

	for (auto fb_index = 0; fb_index < p.frozen_bits_list.size(); fb_index++)
	{
		p.current_frozen_bits = p.frozen_bits_list[fb_index];
		for (auto &m : u.chain->get_modules<tools::Interface_notify_frozenbits_update>())
			m->notify_noise_update();

		// display the legend in the terminal
		if (p.gad->enable_terminal)
		{
			u.terminal->start_temp_report();
		}

		// execute the simulation chain (multi-threaded)
		u.chain->exec([&u]() {return u.monitor_red->is_done() || u.terminal->is_interrupt();});

		// final reduction
		u.monitor_red->reduce();

		// write FER in file
		ofs << std::setprecision(2) << std::scientific << u.monitor_red->get_fer() << std::endl;

		// display the performance (BER and FER) in the terminal
		if (p.gad->enable_terminal)
			u.terminal->final_report();

		// reset the monitor and the terminal for the next SNR
		u.monitor_red->reset();
		u.terminal->reset();

		// if user pressed Ctrl+c twice, exit the SNRs loop
		if (u.terminal->is_over()) break;
	}

	ofs.close();

	// display the statistics of the tasks (if enabled)
	std::cout << "#" << std::endl;
	// tools::Stats::show(m.list, true);
	std::cout << "# End of the simulation" << std::endl;

	return 0;
}
