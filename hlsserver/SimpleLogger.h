#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <string>

namespace lelink{
	class CSimpleLogger{
		CSimpleLogger();
		~CSimpleLogger();

		static CSimpleLogger *mpInstance;
	public:
		static CSimpleLogger *GetLogger();

		//write a line
		void Log(const char *, ...);

	private:
		class CriSection *mpLoggerLock;
		FILE *mpFile;
	};
}