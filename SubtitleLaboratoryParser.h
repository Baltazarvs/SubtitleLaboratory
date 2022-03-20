// 2020 - 2022 Baltazarus

#pragma once
#ifndef PARSER_H
#define PARSER_H
#include "Utilities.h"
#include "Application.h"
#define INDEX_NUMBER			1
#define INDEX_TIME				2
#define INDEX_TEXT				3
#define NEW_LINE_DELIMITER		L'~'

#define NO_FILE_PATH			L"NO_FILE_OPENED"

namespace SubtitleLaboratory
{
	struct hour { };
	struct minute { };
	struct second { };
	struct millisecond { };

	typedef struct
	{
		unsigned int HH, MM, SS, MS;
	} SubRipTimer;

	typedef struct
	{
		unsigned int number;
		SubtitleLaboratory::SubRipTimer time_begin;
		SubtitleLaboratory::SubRipTimer time_end;
		std::wstring lpstrText;
	} SubtitleContainer;



	class SubRipParser
	{
	private:
		std::deque<SubtitleLaboratory::SubtitleContainer> parsed_titles_deque;
		std::size_t titles_number;

	private:
		// For timer
		SubtitleLaboratory::SubRipTimer ParseTime(std::wstring beTimer);

	public:
		SubRipParser();
		~SubRipParser() { }
		std::deque<SubtitleLaboratory::SubtitleContainer> ParseSubtitleFromFile(const wchar_t* path);
		std::pair<SubtitleLaboratory::SubRipTimer, SubtitleLaboratory::SubRipTimer> ParseBeginEndTimers(std::wstring time_line);
		SubtitleLaboratory::SubtitleContainer GenerateContainer(
			unsigned int n, SubtitleLaboratory::SubRipTimer beginTime, 
			SubtitleLaboratory::SubRipTimer endTime, const wchar_t* lpwstrText
		)
		{
			SubtitleLaboratory::SubtitleContainer slc;
			slc.number = n;
			slc.time_begin = beginTime;
			slc.time_end = endTime;
			slc.lpstrText = lpwstrText;

			parsed_titles_deque.push_back(slc);
			return slc;
		}

		std::vector<std::wstring> SplitFilePerBlankLine(const wchar_t* srt_src);
		std::wstring ConvertContainerToString(SubtitleLaboratory::SubtitleContainer cnt_cpy);
		std::deque<SubtitleContainer> GetSubtitlesDeque() const { return this->parsed_titles_deque; }
		std::size_t GetTitlesNumber() const { return this->parsed_titles_deque.size(); }
		SubtitleLaboratory::SubRipTimer ConvertTimerStringToTimerObject(const wchar_t* rHH, const wchar_t* rMM, const wchar_t* rSS, const wchar_t* rMS);
		SubtitleLaboratory::SubRipTimer ValidateTimer(SubtitleLaboratory::SubRipTimer timer_obj);
	
		template <typename RetValue, typename UnitTypeStruct>
		RetValue ConvertToUnit(SubtitleLaboratory::SubRipTimer time);
	};

	template <typename RetValue, typename UnitTypeStruct>
	RetValue SubtitleLaboratory::SubRipParser::ConvertToUnit(SubtitleLaboratory::SubRipTimer time)
	{
		RetValue result = 0u;
		if (std::is_same_v<UnitTypeStruct, SubtitleLaboratory::millisecond>)
		{
			result += (RetValue)time.HH * (3600 * 1000);
			result += (RetValue)time.MM * (60 * 1000);
			result += (RetValue)time.SS * 1000;
			result += (RetValue)time.MS;
		}
		else if (std::is_same_v<UnitTypeStruct, SubtitleLaboratory::second>)
		{
			result += (RetValue)time.HH * 3600;
			result += (RetValue)time.MM * 60;
			result += (RetValue)time.SS;
			result += (RetValue)time.MS / 1000;
		}
		else if (std::is_same_v<UnitTypeStruct, SubtitleLaboratory::minute>)
		{
			result += (RetValue)time.HH * 60;
			result += (RetValue)time.MM;
			result += (RetValue)time.SS / 60;
			result += (RetValue)time.MS / (60 * 1000);
		}
		else if (std::is_same_v<UnitTypeStruct, SubtitleLaboratory::hour>)
		{
			result += (RetValue)time.HH;
			result += (RetValue)time.MM / 60;
			result += (RetValue)time.SS / 3600;
			result += (RetValue)time.MS / (3600 * 1000);
		}
		return result;
	}

	inline void AddTimeToTitle(SubRipTimer& rTimer, int seconds_to_add)
	{
		// Amount of added seconds is less than one hour.
		if (seconds_to_add < 3600)
		{
			// If amount of added seconds is right 60 (1 minute)
			if ((seconds_to_add == 60) || (rTimer.SS == 60))
				rTimer.MM += 1;
			// If amount of added seconds is bigger than 60 but current number of seconds is less than 60 (1 min)
			else if (seconds_to_add > 60)
			{
				// Add amount of minutes that are contained inside seconds_to_add by 60 (1 min)
				rTimer.MM += (seconds_to_add / 60); // E.g. 450 SEC ---> 450 / 60 == 7		00:07:30
				rTimer.SS += (seconds_to_add % 60); // E.g. 450 SEC ---> 450 % 60 == 30		00:07:30
			}
		}
		else
		{
			// Convert seconds to all units.
			rTimer.HH += (seconds_to_add / 3600);
			rTimer.MM += ((seconds_to_add % 3600) / 60);
			rTimer.SS += ((seconds_to_add % 3600) % 60);
		}
		return;
	}

	inline std::string ConvertToString(int n)
	{
		std::stringstream ss;
		ss << n;
		return ss.str();
	}

	inline int ConvertToInt(std::string str)
	{
		int n_temp = 0;
		std::stringstream ss;
		ss >> n_temp;
		return n_temp;
	}
}

#endif