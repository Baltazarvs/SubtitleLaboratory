// 2020 - 2022 Baltazarus

#include "SubtitleLaboratoryParser.h"

SubtitleLaboratory::SubRipTimer SubtitleLaboratory::SubRipParser::ParseTime(std::wstring beTimer)
{
	const wchar_t double_dot_delimiter = L':';
	const wchar_t comma_delimiter = L',';

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

SubtitleLaboratory::SubRipParser::SubRipParser()
	: parsed_titles_deque(std::deque<SubtitleLaboratory::SubtitleContainer>()), titles_number(0u) { }

std::deque<SubtitleLaboratory::SubtitleContainer> SubtitleLaboratory::SubRipParser::ParseSubtitleFromFile(const wchar_t* path)
{
	std::wfstream file(path, std::ios::in | std::ios::out);
	std::wostringstream woss;
	if (file.is_open())
	{
		std::wostringstream complete_file_string;
		complete_file_string << file.rdbuf();

		// Parse per subtitle.
		std::vector<std::wstring> separated_titles_vec = this->SplitFilePerBlankLine(path);

		// Iterate through split titles and parse them one by one.
		for (std::size_t i = 0ull; i < separated_titles_vec.size(); ++i)
		{
			std::wistringstream iss_title(separated_titles_vec[i]);
			std::wstring line;
			int index = INDEX_NUMBER;

			unsigned int title_number = 0u;
			SubtitleLaboratory::SubRipTimer begin_time_obj = { };
			SubtitleLaboratory::SubRipTimer end_time_obj = { };
			std::wostringstream woss_title_text;

			while (std::getline(iss_title, line))
			{
				if (index == INDEX_NUMBER)
				{
					std::wstringstream wss_num;
					wss_num << line;  wss_num >> title_number;
					index = INDEX_TIME; // Set next iteration to be timer section line.
					continue;
				}
				else if (index == INDEX_TIME)
				{
					std::pair<SubtitleLaboratory::SubRipTimer, SubtitleLaboratory::SubRipTimer> parsed_timers = ParseBeginEndTimers(line);
					begin_time_obj = parsed_timers.first;
					end_time_obj = parsed_timers.second;
					index = INDEX_TEXT; // Set next line iteration to be a text section.
					continue;
				}
				else if (index == INDEX_TEXT)
				{
					// TODO:    << '|'    is added! This is last changed thing in this code! FIX IF THERE ARE PROBLEMS!
					woss_title_text << line; // << NEW_LINE_DELIMITER;           IN PROGRESS! Needs function that will replace ~ NEW_LINE_DELIMITER with \n when saving subititle!
					continue;
				}
			}

			SubtitleLaboratory::SubtitleContainer title_container_obj;
			title_container_obj.number = title_number;
			title_container_obj.time_begin = begin_time_obj;
			title_container_obj.time_end = end_time_obj;
			title_container_obj.lpstrText = woss_title_text.str().c_str();
			this->parsed_titles_deque.push_front(title_container_obj);
		}
		file.close();
	}
	else
	{
		MessageBoxA(nullptr, "Cannot open SRT file!", "Error!", MB_OK | MB_ICONERROR);
		return std::deque<SubtitleLaboratory::SubtitleContainer>();
	}
	return this->parsed_titles_deque;
}

// time_line string parameter format must be:	HH:MM:SS,MS --> HH:MM:SS,MS
// Parses given format and returns separated timers: begin and end
std::pair<SubtitleLaboratory::SubRipTimer, SubtitleLaboratory::SubRipTimer> SubtitleLaboratory::SubRipParser::ParseBeginEndTimers(std::wstring time_line)
{
	SubtitleLaboratory::SubRipTimer begin_time_obj;
	SubtitleLaboratory::SubRipTimer end_time_obj;

	std::wstring time_split_delimiter = L" --> ";
	std::size_t begin_time_pos = time_line.find(time_split_delimiter);
	std::wstring begin_time = time_line.substr(0ull, begin_time_pos);

	// Get end time.
	std::wstring end_time = time_line.erase(0ull, begin_time_pos + time_split_delimiter.length());

	// Parse timers and store them into a SubRipTimer structs.
	begin_time_obj = this->ParseTime(begin_time);
	end_time_obj = this->ParseTime(end_time);

	return std::pair<SubtitleLaboratory::SubRipTimer, SubtitleLaboratory::SubRipTimer>(begin_time_obj, end_time_obj);
}

std::vector<std::wstring> SubtitleLaboratory::SubRipParser::SplitFilePerBlankLine(const wchar_t* path)
{
	std::vector<std::wstring> vec_str;
	std::wifstream file;

	file.open(path, std::ios::in);
	if (file.is_open())
	{
		std::wstring line;
		std::wostringstream oss;
		while (std::getline(file, line))
		{
			if (line.length() < 1ull)
			{
				vec_str.push_back(oss.str());
				oss.str(std::wstring());
			}
			else
				oss << line << std::endl;
		}
		// Add last remaining title to vector.
		file.close();
	}
	else
	{
		MessageBoxA(0, "Cannot open specified subtitle!", "SplitFilePerBlankLine() Error", MB_OK | MB_ICONERROR);
		return std::vector<std::wstring>();
	}

	return vec_str;
}

std::wstring SubtitleLaboratory::SubRipParser::ConvertContainerToString(SubtitleLaboratory::SubtitleContainer cnt_cpy)
{
	std::wostringstream woss;
	// Index
	woss << cnt_cpy.number << std::endl;
	// Timers
	woss << cnt_cpy.time_begin.HH <<
		L':' << cnt_cpy.time_begin.MM <<
		L':' << cnt_cpy.time_begin.SS <<
		L',' << cnt_cpy.time_begin.MS;
	woss << L" --> ";
	woss << cnt_cpy.time_end.HH <<
		L':' << cnt_cpy.time_end.MM <<
		L':' << cnt_cpy.time_end.SS <<
		L',' << cnt_cpy.time_end.MS << std::endl;
	// Text
	woss << cnt_cpy.lpstrText << std::endl;
	return woss.str();
}

SubtitleLaboratory::SubRipTimer SubtitleLaboratory::SubRipParser::ConvertTimerStringToTimerObject(const wchar_t* rHH, const wchar_t* rMM, const wchar_t* rSS, const wchar_t* rMS)
{
	SubtitleLaboratory::SubRipTimer subtitle_timer_obj;
	std::wstringstream wss;
	unsigned int HH = 0u, MM = 0u, SS = 0u, MS = 0u;

	wss << rHH; wss >> HH; wss.str(std::wstring()); wss.clear();
	wss << rMM; wss >> MM; wss.str(std::wstring());	wss.clear();
	wss << rSS; wss >> SS; wss.str(std::wstring());	wss.clear();
	wss << rMS; wss >> MS; wss.str(std::wstring());	wss.clear();

	subtitle_timer_obj.HH = HH;
	subtitle_timer_obj.MM = MM;
	subtitle_timer_obj.SS = SS;
	subtitle_timer_obj.MS = MS;

	return subtitle_timer_obj;
}

SubtitleLaboratory::SubRipTimer SubtitleLaboratory::SubRipParser::ValidateTimer(SubtitleLaboratory::SubRipTimer timer_obj)
{
	SubtitleLaboratory::SubRipTimer obj_valid_timer = { };

	if (timer_obj.SS > 60)
	{
		if ((timer_obj.SS % 60) == 0)
			obj_valid_timer.SS = (timer_obj.MM += (timer_obj.SS / 60));
		else
		{
			// In Progress...
		}
	}
	return obj_valid_timer;
}