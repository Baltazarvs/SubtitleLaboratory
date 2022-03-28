#include "VTTSubtitleParser.h"

SubtitleLaboratory::SubRipTimer SubtitleLaboratory::VTTSubtitleParser::ParseTime(std::wstring beTimer)
{
	const wchar_t double_dot_delimiter = L':';
	const wchar_t comma_delimiter = L'.';

	SubtitleLaboratory::SubRipTimer parsed_time_obj;
	std::vector<std::wstring> tokens;							// Will store HH, MM, SS, MS strings respectively.
	std::wstringstream wss_HH, wss_MM, wss_SS, wss_MS;			// Will be used for converting string values into a unsigned int.
	unsigned int HH = 0u, MM = 0u, SS = 0u, MS = 0u;

	// Separater HH:MM:SS from MS with delimiter ,
	std::wstring begin_time_separated = std::wstring();			// Begin HH:MM:SS will be stored here after parsing.
	std::size_t c_position = beTimer.find(comma_delimiter);
	if (c_position != std::wstring::npos)
		begin_time_separated = beTimer.substr(0ull, c_position);


	// Parse HH:MM:SS and store values into the struct.
	std::size_t hhmmss_pos = 0ull;
	while ((hhmmss_pos = begin_time_separated.find(double_dot_delimiter)) != std::wstring::npos)
	{
		std::wstring unit_result = begin_time_separated.substr(0, hhmmss_pos);
		tokens.push_back(unit_result);
		begin_time_separated.erase(0ull, hhmmss_pos + 1ull);
	}

	tokens.push_back(begin_time_separated);
	beTimer.erase(0ull, c_position + 1ull);
	tokens.push_back(beTimer);

	// Convert strings to integers that will be stored inside parsed_time_obj
	wss_HH << tokens[0]; wss_HH >> HH;
	wss_MM << tokens[1]; wss_MM >> MM;
	wss_SS << tokens[2]; wss_SS >> SS;
	wss_MS << tokens[3]; wss_MS >> MS;

	// Store values inside struct.
	parsed_time_obj.HH = HH;
	parsed_time_obj.MM = MM;
	parsed_time_obj.SS = SS;
	parsed_time_obj.MS = MS;

	// Return timer.
	return parsed_time_obj;
}
