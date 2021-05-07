/*!
 * \file
 * \brief Methods to generate frozen bits list
 */
#ifndef GEN_FB_LIST_HPP
#define GEN_FB_LIST_HPP

#include "aff3ct_init.hpp"


bool gen_fb_list_from_file(params &p);
bool gen_fb_list_shuffle  (params &p, tools::Noise<>& noise);
bool gen_fb_polar_rm      (params &p, tools::Noise<>& noise);

#endif
