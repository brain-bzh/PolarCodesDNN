#include "aff3ct_init.hpp"

void init_params(int argc, char** argv, params &p)
{
	p.source   = std::unique_ptr<factory::Source      >(new factory::Source      ());
	p.crc      = std::unique_ptr<factory::CRC         >(new factory::CRC         ());
	p.codec    = std::unique_ptr<factory::Codec_polar >(new factory::Codec_polar ());
	p.modem    = std::unique_ptr<factory::Modem       >(new factory::Modem       ());
	p.channel  = std::unique_ptr<factory::Channel     >(new factory::Channel     ());
	p.monitor  = std::unique_ptr<factory::Monitor_BFER>(new factory::Monitor_BFER());
	p.terminal = std::unique_ptr<factory::Terminal    >(new factory::Terminal    ());
	p.gad      = std::unique_ptr<factory::Gad         >(new factory::Gad         ());

	std::vector<factory::Factory*> params_list = { p.source .get(), p.codec  .get(), p.modem   .get(),
	                                               p.channel.get(), p.monitor.get(), p.terminal.get(),
												   p.crc.get(), p.gad.get()                           };

	// parse the command for the given parameters and fill them
	tools::Command_parser cp(argc, argv, params_list, true);
	if (cp.parsing_failed() || cp.help_required())
	{
		cp.print_help    ();
		cp.print_warnings();
		cp.print_errors  ();
		std::exit(1);
	}

	p.crc    ->K = p.source->K - p.crc->size;
	p.source ->K = p.crc   ->K;
	p.monitor->K = p.source->K;

    // initialize dummy frozen bits
    p.current_frozen_bits = std::vector<bool>(p.codec->enc->N_cw,1);
    std::fill(p.current_frozen_bits.begin(), p.current_frozen_bits.begin() + p.codec->enc->K, 0);

	std::cout << "# Simulation parameters: " << std::endl;
	tools::Header::print_parameters(params_list); // display the headers (= print the AFF3CT parameters on the screen)
	std::cout << "#" << std::endl;
	cp.print_warnings();

	p.source->seed  = p.gad->seed;
	p.channel->seed = p.gad->seed;
	p.R = (float)p.source->K / (float)p.codec->enc->N_cw; // compute the code rate
}

void init_modules(const params &p, modules &m)
{
	auto enc_polar = dynamic_cast<factory::Encoder_polar*>(p.codec->enc.get());
	auto dec_polar = dynamic_cast<factory::Decoder_polar*>(p.codec->dec.get());
	m.source  = std::unique_ptr<module::Source      <>>(p.source ->build(                                  ));
	m.modem   = std::unique_ptr<module::Modem       <>>(p.modem  ->build(                                  ));
	m.channel = std::unique_ptr<module::Channel     <>>(p.channel->build(                                  ));
	m.monitor = std::unique_ptr<module::Monitor_BFER<>>(p.monitor->build(                                  ));
	m.crc     = std::unique_ptr<module::CRC         <>>(p.crc    ->build(                                  ));
	m.encoder = std::unique_ptr<module::Encoder     <>>(enc_polar->build(p.current_frozen_bits             ));
	m.decoder = std::unique_ptr<module::Decoder_SIHO<>>(dec_polar->build(p.current_frozen_bits, m.crc.get()));
}

void init_utils(const params &p, const modules &m, utils &u)
{
	// create chain
	u.chain = std::unique_ptr<tools::Chain>(new tools::Chain((*m.source)[module::src::tsk::generate],
	p.gad->n_threads ? p.gad->n_threads : 1));
	// allocate a common monitor module to reduce all the monitors
	u.monitor_red = std::unique_ptr<tools::Monitor_BFER_reduction>(new tools::Monitor_BFER_reduction(
		u.chain->get_modules<module::Monitor_BFER<>>()));
	u.monitor_red->set_reduce_frequency(std::chrono::milliseconds(500));
	// create a sigma noise type
	u.noise = std::unique_ptr<tools::Sigma<>>(new tools::Sigma<>());
	// report the noise values (Es/N0 and Eb/N0)
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));
	// report the bit/frame error rates
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER<>(*u.monitor_red)));
	// report the simulation throughputs
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*u.monitor_red)));
	// create a terminal that will display the collected data from the reporters
	u.terminal = std::unique_ptr<tools::Terminal>(p.terminal->build(u.reporters));
	// configuration of the chain tasks
	for (auto& mod : u.chain->get_modules<module::Module>(false))
		for (auto& tsk : mod->tasks)
		{
			tsk->set_debug      (false); // disable the debug mode
			tsk->set_debug_limit(16   ); // display only the 16 first bits if the debug mode is enabled
			tsk->set_stats      (true ); // enable the statistics

			// enable the fast mode (= disable the useless verifs in the tasks) if there is no debug and stats modes
			if (!tsk->is_debug() && !tsk->is_stats())
				tsk->set_fast(true);
		}
}
