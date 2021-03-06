// device.cpp: implementation of the CDevice class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sniffusb.h"
#include "device.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDevice::CDevice()
{
	m_hDev = NULL;
	memset(&m_hDevInfo, 0, sizeof(m_hDevInfo));
	b_IsPresent = FALSE;
}

CDevice::CDevice(HDEVINFO hDev,SP_DEVINFO_DATA devInfo)
{
	m_hDev = hDev;
	m_hDevInfo = devInfo;
	b_IsPresent = FALSE;
}

CDevice::~CDevice()
{
}

BOOL CDevice::Restart()
{
	SP_PROPCHANGE_PARAMS params;

	// initialize the structure

	memset(&params, 0, sizeof(params));

	// initialize the SP_CLASSINSTALL_HEADER struct at the beginning
	// of the SP_PROPCHANGE_PARAMS struct, so that
	// SetupDiSetClassInstallParams will work

	params.ClassInstallHeader.cbSize = sizeof(params.ClassInstallHeader);
	params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

	// initialize SP_PROPCHANGE_PARAMS such that the device will be
	// stopped.

	params.StateChange = DICS_PROPCHANGE;
	params.Scope       = DICS_FLAG_CONFIGSPECIFIC;
	params.HwProfile   = 0; // current profile

	if(!SetupDiSetClassInstallParams(m_hDev, &m_hDevInfo,
		(PSP_CLASSINSTALL_HEADER) &params,sizeof(SP_PROPCHANGE_PARAMS)))
	{
		return FALSE;
	}

	// we call SetupDiCallClassInstaller() twice for the ECI USB ADSL
	// modem, maybe because it is 2 USB devices. But this is not
	// needed for all USB devices, so it's better for the end user to
	// hit the button "replug" twice.

	if(!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, m_hDev, &m_hDevInfo))
	{
		return FALSE;
	}
	
	return TRUE;
}

void Perror(DWORD dwLastError, LPCTSTR str)
{
	char msg [100];

	sprintf_s(msg, sizeof(msg), "GetLastError() = %d\n",dwLastError);
	MessageBox(NULL,msg,str,MB_OK);
}

const CMultiSz& CDevice::GetLowerFilters()
{
	DWORD DataType, BufferSize = 0;
	BYTE * Buffer = NULL;
	BOOL result;

	static int once = 0;

	// warning: on windows xp, the following call will alway
	// return FALSE and set GetLastError() = 13
	// [ERROR_INVALID_DATA].

	result = SetupDiGetDeviceRegistryProperty(m_hDev,&m_hDevInfo,SPDRP_LOWERFILTERS,
		&DataType,Buffer,BufferSize,&BufferSize);

	if (BufferSize != 0)
	{
		// we allocate the proper buffer

		Buffer = (BYTE *) malloc(BufferSize);

		// we get the content of the buffer

		if (SetupDiGetDeviceRegistryProperty(m_hDev,&m_hDevInfo,SPDRP_LOWERFILTERS,
			&DataType,Buffer,BufferSize,NULL))
		{
			// DataType is REG_MULTI_SZ
			m_LowerFilters.SetBuffer(Buffer, BufferSize);
/*
			if (!once && BufferSize > 0)
			{
				once = 1;

				char msg[100];
				sprintf(msg,"DataType = %d, BufferSize = %d",
					DataType, BufferSize);
				MessageBox(NULL,msg,"CDevice::GetLowerFilters",MB_OK);

				m_LowerFilters.DisplayBuffer();
			}
*/
		}
		else
		{
			TRACE("SetupDiGetDeviceRegistryProperty [%s] = KO\n",m_DeviceDesc);
/*
			Perror(GetLastError(),m_FriendlyName);

			char msg[100];
			sprintf(msg,"DataType = %d, BufferSize = %d",
				DataType, BufferSize);
			MessageBox(NULL,msg,"CDevice::GetLowerFilters",MB_OK);
*/
			m_LowerFilters.SetBuffer(Buffer, BufferSize);
			//m_LowerFilters.DisplayBuffer();
		}

		free (Buffer);
	}
	else
	{
		m_LowerFilters.SetBuffer(Buffer, BufferSize);
	}

	return m_LowerFilters;
}

void CDevice::SetLowerFilters(const CMultiSz& sz)
{
    // check if we can remove the key in the registry

    if (sz.GetBufferLen() > 2)
    {
        if (!SetupDiSetDeviceRegistryProperty(m_hDev,&m_hDevInfo,SPDRP_LOWERFILTERS ,
            sz.GetBuffer(),sz.GetBufferLen()))
        {
            MessageBox(NULL,_T("Install failed!"),_T("SetLowerFilters"),MB_OK);
        }
    }
    else
    {
        if (!SetupDiSetDeviceRegistryProperty(m_hDev,&m_hDevInfo,SPDRP_LOWERFILTERS ,
            NULL,0))
        {
            // windows xp:
            // if the entry has already been deleted, the function will fail with
            // GetLastError() = ERROR_INVALID_DATA. strange :-(

            BYTE sEmpty[] = { 0, 0, 0, 0, 0, 0 };

            if(!SetupDiSetDeviceRegistryProperty(m_hDev, &m_hDevInfo, SPDRP_LOWERFILTERS,
                sEmpty, 2 * sizeof(TCHAR)))
            {
                DWORD dwLastError = GetLastError();

                if (dwLastError != NO_ERROR)
                {
                    char msg [100];
                    sprintf_s(msg, sizeof(msg),"GetLastError() = %d",dwLastError);
                    MessageBox(NULL,msg,_T("SetLowerFilters"),MB_OK);
                }
            }
        }
    }
}
