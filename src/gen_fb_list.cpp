#include <numeric>

#include "gen_fb_list.hpp"

template<typename T>
T string_to_value(std::string value_str)
{
	T value;
	try
	{
		std::istringstream iss(value_str);
		iss.exceptions(std::ios::failbit | std::ios::badbit);
		iss >> value;
	}
	catch(std::exception&)
	{
		std::stringstream message;
		message << "conversion did not work, something went wrong when reading the file.";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}

	return value;
}

template<typename T>
void string_to_vector(std::string vector_str, std::vector<T>& vector)
{
	T value;
	try
	{
		std::istringstream iss(vector_str);
		iss.exceptions(std::ios::failbit | std::ios::badbit);
		for (size_t i = 0; i < vector.size(); i++)
		{
			iss >> value;
			vector[i] = value;
		}
	}
	catch(std::exception&)
	{
		std::stringstream message;
		message << "conversion did not work, something went wrong when reading the file.";
		throw runtime_error(__FILE__, __LINE__, __func__, message.str());
	}
}


bool gen_fb_list_from_file(params &p)
{
	std::string line_read;
	std::string filename(p.gad->fb_file);
	std::ifstream in_code(filename.c_str());

	if (in_code.is_open())
	{
		std::vector<bool> local_fb(p.codec->enc->N_cw);

		for (auto i = 0; i < p.gad->dataset_size; i++)
		{
			if (in_code.eof())
				throw tools::runtime_error(__FILE__, __LINE__, __func__, "Reached eof.");
			std::getline(in_code, line_read);
			string_to_vector(line_read, local_fb);
			p.frozen_bits_list.push_back(local_fb);
		}

		p.current_frozen_bits = p.frozen_bits_list[0];

		in_code.close();
		return true;
	}
	else
		return false;
}

bool gen_fb_list_shuffle(params &p, tools::Noise<>& noise)
	{
	auto enc = p.codec->enc;
	auto pivot = enc->K;
	std::mt19937 g;
	g.seed(p.gad->seed);
	std::vector<bool> frozen_bits(enc->N_cw);
	Frozenbits_generator_GA_Arikan fbg(enc->K, enc->N_cw);

	fbg.set_noise(noise);
	fbg.generate(frozen_bits);
	auto best_channels = fbg.get_best_channels();

	// shuffle, store in frozen_bits_list
	p.frozen_bits_list.push_back(frozen_bits);
	for (auto i = 0; i < p.gad->dataset_size -1; i++)
	{
		std::vector<bool> frozen_bits_shuffled(enc->N_cw,true);
		std::shuffle(best_channels.begin() + enc->K - p.gad->shuffle_range,
		             best_channels.begin() + enc->K + p.gad->shuffle_range,
		             g);
		for (auto i = 0u; i < enc->K; i++)
			frozen_bits_shuffled[best_channels[i]] = false;
		p.frozen_bits_list.push_back(frozen_bits_shuffled);
	}

	// write fb in a file
	std::ofstream ofs;
	ofs.open ("fb" + std::to_string(p.gad->seed) + ".txt", std::ofstream::out | std::ofstream::trunc);
	if (!ofs.is_open())
		throw runtime_error(__FILE__, __LINE__, __func__, std::string("Unable to open fer.txt."));

	for (auto i = 0u; i < p.frozen_bits_list.size(); i++)
	{
		for (auto j = 0u; j < enc->N_cw; j++)
			ofs << p.frozen_bits_list[i][j] << " ";
		ofs << std::endl;
	}

	ofs.close();
	}

std::vector<std::vector<int>> kron_pow(std::vector<std::vector<int>> matrix)
{
	const auto N = matrix.size();
	for (unsigned i = 0; i < N; i++)
	{
		matrix.push_back(matrix[i]);
	}
	for (unsigned i = 0; i < N; i++)
	{
		matrix[i].resize(matrix[i].size() * 2);
		std::fill(matrix[i].begin() + matrix[i].size() / 2, matrix[i].end(), 0);
		matrix[i + N].insert(matrix[i + N].end(), matrix[i + N].begin(), matrix[i + N].end());
	}
	return matrix;
}

void generate_row_weights(std::vector<int>& row_weights)
{
	const auto N = row_weights.size();
	auto count = 0;
	std::vector<std::vector<int>> kernel = {{1,0},{1,1}};
	auto matrix = kernel;

	while (matrix.size() != row_weights.size())
		matrix = kron_pow(matrix);

	for (unsigned i = 0; i < row_weights.size(); i++)
		row_weights[i] = std::accumulate(matrix[i].begin(), matrix[i].end(), 0);
}

bool gen_fb_polar_rm(params &p, tools::Noise<>& noise)
{
	auto enc = p.codec->enc;
	auto pivot = enc->K;
	std::mt19937 g;
	g.seed(p.gad->seed);
	std::vector<bool> frozen_bits(enc->N_cw);
	Frozenbits_generator_GA_Arikan fbg(enc->K, enc->N_cw);

	fbg.set_noise(noise);
	fbg.generate(frozen_bits);
	auto best_channels = fbg.get_best_channels();
	auto best_channels_rm = best_channels;
	std::vector<int> row_weights(enc->N_cw);
	generate_row_weights(row_weights);
	auto cur_idx = enc->N_cw - 1;
	for (auto i = enc->N_cw - 1; i >= 0 ; i--)
		if (row_weights[best_channels[i]] <= 16)
			best_channels_rm[cur_idx--] = best_channels[i];

	for (auto i = enc->N_cw - 1; i >= 0 ; i--)
		if (row_weights[best_channels[i]] > 16)
			best_channels_rm[cur_idx--] = best_channels[i];

	if (cur_idx != -1)
		throw tools::runtime_error("Something went wrong...");

	best_channels = best_channels_rm;
	std::fill(frozen_bits.begin(), frozen_bits.end(), true);
	for (auto i = 0u; i < enc->K; i++)
		frozen_bits[best_channels[i]] = false;
	// shuffle, store in frozen_bits_list
	p.frozen_bits_list.push_back(frozen_bits);
	for (auto i = 0; i < p.gad->dataset_size -1; i++)
	{
		std::vector<bool> frozen_bits_shuffled(enc->N_cw,true);
		std::shuffle(best_channels.begin() + enc->K - p.gad->shuffle_range,
		             best_channels.begin() + enc->K + p.gad->shuffle_range,
		             g);
		for (auto i = 0u; i < enc->K; i++)
			frozen_bits_shuffled[best_channels[i]] = false;
		p.frozen_bits_list.push_back(frozen_bits_shuffled);
	}

	// write fb in a file
	std::ofstream ofs;
	ofs.open ("fb" + std::to_string(p.gad->seed) + ".txt", std::ofstream::out | std::ofstream::trunc);
	if (!ofs.is_open())
		throw runtime_error(__FILE__, __LINE__, __func__, std::string("Unable to open fer.txt."));

	for (auto i = 0u; i < p.frozen_bits_list.size(); i++)
	{
		for (auto j = 0u; j < enc->N_cw; j++)
			ofs << p.frozen_bits_list[i][j] << " ";
		ofs << std::endl;
	}

	ofs.close();
}
