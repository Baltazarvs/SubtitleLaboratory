// Created 2021 - 2022 Baltazarus

#pragma once
#ifndef VTT_PARSER_H
#define VTT_PARSER_H
#include "SubtitleLaboratoryParser.h"

namespace SubtitleLaboratory
{
	class VTTSubtitleParser : public SubtitleLaboratory::SubRipParser
	{
	private:
		SubtitleLaboratory::SubRipTimer ParseTime(std::wstring beTimer) override;
	};
}

#endif