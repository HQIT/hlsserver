#pragma once

namespace lelink{
	class CriSection{
		CRITICAL_SECTION m_critclSection;
	public:
		CriSection::CriSection(){  
			::InitializeCriticalSection(&m_critclSection);  
		}

		virtual CriSection::~CriSection(){  
			::DeleteCriticalSection(&m_critclSection);  
		}  

		void CriSection::Lock() const{
			::EnterCriticalSection((LPCRITICAL_SECTION)&m_critclSection);  
		}     

		void CriSection::Unlock() const{
			::LeaveCriticalSection((LPCRITICAL_SECTION)&m_critclSection);  
		} 
	};
}