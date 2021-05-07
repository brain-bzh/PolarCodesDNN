/*!
 * \file
 * \brief Init methods for aff3ct params, modules, utils
 */
#ifndef AFF3CT_INIT_HPP
#define AFF3CT_INIT_HPP

#include <memory>

#include <aff3ct.hpp>
#include "Factory/Gad.hpp"

using namespace aff3ct;

struct params
{
	float R;                  // code rate (R=K/N)

	std::vector<std::vector<bool>> frozen_bits_list;
	std::vector<bool> current_frozen_bits;

	std::unique_ptr<factory::Source      > source;
	std::unique_ptr<factory::Codec_polar > codec;
	std::unique_ptr<factory::CRC         > crc;
	std::unique_ptr<factory::Modem       > modem;
	std::unique_ptr<factory::Channel     > channel;
	std::unique_ptr<factory::Monitor_BFER> monitor;
	std::unique_ptr<factory::Terminal    > terminal;
	std::unique_ptr<factory::Gad         > gad;
};
void init_params(int argc, char** argv, params &p);

struct modules
{
	std::unique_ptr<module::Source<>      > source;
	std::unique_ptr<module::Modem<>       > modem;
	std::unique_ptr<module::Channel<>     > channel;
	std::unique_ptr<module::Monitor_BFER<>> monitor;
	std::unique_ptr<module::CRC<>         > crc;
	std::unique_ptr<module::Encoder<>     > encoder;
	std::unique_ptr<module::Decoder_SIHO<>> decoder;
};
void init_modules(const params &p, modules &m);


namespace aff3ct { namespace tools {
using Monitor_BFER_reduction = Monitor_reduction<module::Monitor_BFER<>>;
} }

struct utils
{
	            std::unique_ptr<tools::Sigma<>  > noise;     // a sigma noise type
	std::vector<std::unique_ptr<tools::Reporter>> reporters; // list of reporters dispayed in the terminal
	            std::unique_ptr<tools::Terminal > terminal;  // manage the output text in the terminal
	            std::unique_ptr<tools::Monitor_BFER_reduction>  monitor_red; // main monitor object that reduce all the thread monitors
	            std::unique_ptr<tools::Chain                 >  chain;
};
void init_utils(const params &p, const modules &m, utils &u);

#endif /* AFF3CT_INIT_HPP */
