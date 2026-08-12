/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CMESSAGEADC_H
#define CMESSAGEADC_H

#include "cprotocol.h"

#include <string>
#include <vector>

namespace nVerliHub {
	namespace nEnums {
		typedef enum
		{
			eADC_STA,
			eADC_SUP,
			eADC_SID,
			eADC_INF,
			eADC_MSG,
			eADC_SCH,
			eADC_RES,
			eADC_CTM,
			eADC_RCM,
			eADC_QUI,
			eADC_GPA,
			eADC_PAS,
			eADC_GET,
			eADC_GFI,
			eADC_SND,
			eADC_UNKNOWN,
			eADC_INVALID
		} tADCMsg;
	}

	namespace nProtocol {

/**
 * ADC protocol message parser.
 *
 * The transport removes the terminating LF before assigning mStr. This class
 * validates the ADC message header, extracts routing identifiers and decodes
 * ADC escaping (\\s, \\n and \\\\).
 */
class cMessageADC : public cMessageParser
{
	public:
		cMessageADC();
		virtual ~cMessageADC();

		virtual int Parse();
		virtual bool SplitChunks();
		virtual void ReInit();

		char HeaderType() const { return mHeaderType; }
		const std::string &Command() const { return mCommand; }
		const std::string &SourceSID() const { return mSourceSID; }
		const std::string &TargetSID() const { return mTargetSID; }
		const std::string &SourceCID() const { return mSourceCID; }
		const std::vector<std::string> &Features() const { return mFeatures; }
		const std::vector<std::string> &Parameters() const { return mParameters; }

		bool GetNamed(const std::string &name, std::string &value) const;

		static bool Escape(const std::string &src, std::string &dest);
		static bool UnEscape(const std::string &src, std::string &dest);
		static bool IsSID(const std::string &sid);
		static bool IsCID(const std::string &cid);
		static bool IsFeatureSelector(const std::string &features);
		static int CommandType(const std::string &command);

	private:
		bool Tokenize(std::vector<std::string> &raw, std::vector<size_t> *offsets = NULL) const;
		bool ParseHeader(std::vector<std::string> &raw, size_t &firstParameter);

		char mHeaderType;
		std::string mCommand;
		std::string mSourceSID;
		std::string mTargetSID;
		std::string mSourceCID;
		std::vector<std::string> mFeatures;
		std::vector<std::string> mParameters;
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
