#pragma once

#include "Ticker.h"
using namespace com::cloume::common;

namespace com{
	namespace cloume{
		namespace cap{

			class WinTicker : public Ticker{
			public:
				WinTicker();

				virtual unsigned int GetCurrentTickCount();
			};
		}
	}
}