/*
    How to use:
    CiniHelp ini;
    ini.Open()

history:
	2018-11-26 10:18
	fixed drop commented content for inline.

	28-11-20 14:27
	fixed write section key to the end of the section. 


*/
#ifndef __INIHELP_HEADER__
#define __INIHELP_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>
#include <string>

#ifndef WIN32  
#ifndef MAX_PATH
#define MAX_PATH   300
#endif//MAX_PATH
typedef long long  LONGLONG;
#else
#endif

#define MAX_FILE_SIZE 1024*16
#define LEFT_BRACE '['
#define RIGHT_BRACE ']'

class CINIHelp
{
public:
    CINIHelp()
    {
        m_szSection[0]='\0';
        m_szFile[0]='\0';
        m_nFileSize = 0;
        m_lpszBuffer=0;
    }

    CINIHelp(const char* lpszFile,const char* lpszSection=0)
    {
        m_szSection[0]='\0';
        m_szFile[0]='\0';
        m_nFileSize = 0;
        m_lpszBuffer=0;

        Open(lpszFile,lpszSection);
    }

    ~CINIHelp()
    {
        FreeBuffer();
    }

public://try to open the special file.
    int Open(const char* lpszFile,const char* lpszSection=NULL)
    {
		int bSucc = 0;
        int nStatus = load_ini_file(lpszFile);
        strncpy(m_szFile,lpszFile,MAX_PATH);
        if ((0 == nStatus) && (lpszSection)) {
            strncpy(m_szSection,lpszSection,sizeof(m_szSection));
        }

		// check if succeeded.
		if (0 == nStatus) {
			bSucc = 1;
		}
    //Cleanup:
        return (bSucc);
    }

public:
    int IsOpened()
    {
        return (m_lpszBuffer!=NULL);
    }

    void SetSection(const char* lpszSection)
    {
        strcpy(m_szSection,lpszSection);
    }

    const char* GetString(const char* lpszKeyName)
    {
        return (GetIniKeyString(m_szSection,lpszKeyName));
    }

    int SetString(const char* lpszKeyName,const char* lpszValue)
    {
        return (WriteIniKeyString(m_szSection,lpszKeyName,lpszValue));
    }

    int GetLong(const char* lpszKeyName,int ndefVal=0)
    {
        return (GetIniKeyInt(m_szSection,lpszKeyName));
    }

    int SetLong(const char* lpszKeyName,int ndefVal)
    {
        char buf[32]={0};
        sprintf(buf,"%ld",ndefVal);
        if (!strlen(buf)) {
            buf[0]='0';
        }
    //Cleanup:
        return (WriteIniKeyString(m_szSection,lpszKeyName,buf));
    }

    const char* GetString(const char* lpszSection,const char* lpszKeyName)
    {
        return (GetIniKeyString(lpszSection,lpszKeyName));
    }

    const char* GetString(const char* lpszSection,const char* lpszKeyName,char* lpszBuffer,int nBufSize)
    {
        if (lpszBuffer) lpszBuffer[0]='\0';
        GetIniKeyString(lpszSection,lpszKeyName,lpszBuffer,nBufSize);
    //Cleanup:
        return (lpszBuffer);
    }

    float GetFloat(const char* lpszSection, const char* lpszKeyName,float defVal=0)
    {
        float retVal = defVal;
        const char* p = GetString(lpszSection,lpszKeyName);
        if (p) {
            retVal = (float)atof(p);
        }
    //Cleanup:
        return (retVal);
    }

    int SetString(const char* lpszSection, const char* lpszKeyName, const char* lpszValue)
    {
        return (WriteIniKeyString(lpszSection,lpszKeyName,lpszValue));
    }

    int GetLong(const char* lpszSection, const char* lpszKeyName, int ndefVal=0)
    {
        return (GetIniKeyInt(lpszSection,lpszKeyName,ndefVal));
    }

    int SetLong(const char* lpszSection, const char* lpszKeyName,int ndefVal)
    {
        char buf[32]={0};
        sprintf(buf,"%ld",ndefVal);
        if (!strlen(buf)) {
            buf[0]='0';
        }
    //Cleanup:
        return (WriteIniKeyString(lpszSection,lpszKeyName,buf));
    }

    LONGLONG GetLongI64(const char* lpszSection,const char* lpszKeyName,LONGLONG ndefVal=0)
    {
        LONGLONG retVal = ndefVal;
        const char* p = GetString(lpszSection,lpszKeyName);
        if (p)
        {
#ifndef WIN32
            retVal = strtol(p, 0, 10);
#else
            retVal = _atoi64(p);
#endif//WIN32
        }
    //Cleanup:
        return (retVal);
    }

    int SetLongI64(const char* lpszSection,const char* lpszKeyName,LONGLONG ndefVal)
    {
        char buf[32]={0};
#ifndef WIN32//llu.
        sprintf(buf,"%lld",ndefVal);
#else
        sprintf(buf,"%I64d",ndefVal);
#endif//WIN32
        if (!strlen(buf)) {
            buf[0]='0';
        }
    //Cleanup:
        return (WriteIniKeyString(lpszSection,lpszKeyName,buf));
    }

public:
	static bool WritePrivateProfileString(const char* pszSection, const char* pszKeyName, const char* pszValue, const char* pszFileName)
	{
		CINIHelp inihelp;
		bool bSuccess = false;
		do
		{
			// try to open the special file item value.
			if (inihelp.Open(pszFileName,pszSection))
			{
				bSuccess = inihelp.SetString(pszKeyName,pszValue);	
			}

		} while (0);
	//Cleanup:
		return (bSuccess);
	}

	static bool GetPrivateProfileString(const char* pszSection, const char* pszKeyName, const char* pszDefault,char* pszBuffer,int nBufSize,const char* pszFileName)
	{
		CINIHelp inihelp;
		bool bSuccess = false;
		do
		{
			if (inihelp.Open(pszFileName,pszSection))
			{
				lstrcpyn(pszBuffer,pszDefault,nBufSize);
				LPCTSTR lpsz = inihelp.GetString(pszKeyName);
				if (lstrlen(lpsz)) {
					lstrcpyn(pszBuffer,lpsz,nBufSize);
					bSuccess = true;
				}
			}

		} while (0);
	//Cleanup:
		return (bSuccess);
	}

protected:
    void FreeBuffer()
    {
        // check the buffer.
        if (m_lpszBuffer) {
            free(m_lpszBuffer);
            m_lpszBuffer=NULL;
        }
    }

protected: // 内部文件操作.
    static int newline(char c)
    {
        return ('\n' == c ||  '\r' == c )? 1 : 0;
    }
    
    static int end_of_string(char c)
    {
        return '\0'==c? 1 : 0;
    }
    
    static int left_barce(char c)
    {
        return LEFT_BRACE == c? 1 : 0;
    }
    
    static int isright_brace(char c )
    {
        return RIGHT_BRACE == c? 1 : 0;
    }

    static int parse_file(const char *section, const char *key, const char *buf,int *sec_start,int *sec_end,
        int *key_start,int *key_end, int *value_start, int *value_end)
    {
        const char *p = buf;
        int keysize = lstrlen(key);
        int i=0,keysize2=0;

        assert(buf!=NULL);
        assert(section != NULL && strlen(section));
        assert(key != NULL && keysize);

        *sec_end = *sec_start = *key_end = *key_start = *value_start = *value_end = -1;

        while( !end_of_string(p[i]) ) {
            //find the section
            if( ( 0==i ||  newline(p[i-1]) ) && left_barce(p[i]) )
            {
                int section_start=i+1;

                //find the ']'
                do {
                    i++;
                } while( !isright_brace(p[i]) && !end_of_string(p[i]));

                if (0 == strncmp(p+section_start,section, i-section_start))
                {
                    int newline_start=0;

                    i++;

                    //Skip over space char after ']'
                    while(isspace(p[i])) {
                        i++;
                    }

                    // find the section item now. 
                    *sec_start = section_start;
                    *sec_end = i;

                    // while if not the next section start, just find all the key item now.
                    while(!(newline(p[i-1]) && left_barce(p[i])) && !end_of_string(p[i]))
                    {
                            int j=0;
                            // get a new line
                            newline_start = i;

                            while( !newline(p[i]) &&  !end_of_string(p[i]) ) {
                                i++;
                            }

                            //now i  is equal to end of the line
                            j = newline_start;
                            keysize2 = 0;

                            //skip over comment.
                            if ((';' != p[j]) && 
                                ('#' != p[j]) && 
                                ('/' != p[j]))
                            {
                                while(j < i && p[j]!='=') {
                                    j++;
                                    keysize2++;
                                    if('=' == p[j]) 
                                    {
                                        if (keysize>keysize2) keysize2=keysize;
                                        if(strncmp(key,p+newline_start,keysize2)==0)
                                        {
                                            //find the key ok
                                            *key_start = newline_start;
                                            *key_end = j-1;

                                            *value_start = j+1;
                                            *value_end = i;

                                            return 1;
                                        }
                                    }
                                }
                            }

                            i++;
                            // update the section end to section last.
                            *sec_end = i;
                    }
                }
            }
            else
            {
                i++;
            }
        }
        return 0;
    }

    /**
    *@brief read string fin initialization file\n
    * retrieves a string from the specified section fin an initialization file
    *@param section [fin] name of the section containing the key name
    *@param key [fin] name of the key pairs to value 
    *@param value [fin] pointer to the buffer that receives the retrieved string
    *@param size [fin] size of result's buffer 
    *@param default_value [fin] default value of result
    *@param file [fin] path of the initialization file
    *@return 1 : read success; \n 0 : read fail
    */
    const char* GetIniKeyString( const char *section, const char *key,char* inbuf=NULL,int nBufSize=0)
    {
        static char defval[3]="";
        static char tmpbuf[1024]={0};
        int sec_start,sec_end,key_start,key_end, value_start, value_end;
        char* buf   = m_lpszBuffer;
        char* value = tmpbuf;
        if (inbuf) {
            value=inbuf;
        }

	// empty now.
	value[0]='\0';
	/*
        //check parameters
        assert(m_lpszBuffer !=NULL &&strlen(key));
        assert(section != NULL && strlen(section));
        assert(key != NULL && strlen(key));
	*/

        if(!m_lpszBuffer)
        {
            strncpy(value,defval, sizeof(value));
            return value;
        }

        // try to parse the special buffer content to find the special content value.
        if(!parse_file(section,key,buf,&sec_start,&sec_end,&key_start,&key_end,&value_start,&value_end))
        {
            strncpy(value,defval, sizeof(value));
            return value; //not find the key
        }
        else
        {
            int cpcount = value_end - value_start;
            if( sizeof(tmpbuf) < cpcount)
            {
                cpcount = sizeof(tmpbuf)-1;
            }

            memset(value, 0, sizeof(value));
            memcpy(value,buf+value_start, cpcount);
            value[cpcount] = '\0';

            // for skip the commented ";" data.
            for (int i=0;i<cpcount;i++) {
                if ((value[i] == ';')||
                    (value[i] == ' ')||
                    (value[i] == 0x09))
                {
                    value[i] = '\0';
                    break;
                }
            }

            return value;
        }
    }

    // try to read the special section from the special content item value item now.
    int GetIniKeyInt(const char *lpszSection, const char *lpszKey,int default_value=0)
    {
        const char* p = GetIniKeyString(lpszSection,lpszKey);
        if(!p)
        {
            return default_value;
        }
        else
        {
            return atoi(p);
        }
    }

    // try to write the special content to the special INI file content value now.
    int WriteIniKeyString(const char *section, const char *key,const char *value)
    {
	    char w_buf[MAX_FILE_SIZE]={0};
	    int sec_start,sec_end,key_start,key_end, value_start, value_end;
	    int value_len = (int)strlen(value);
            char* file = m_szFile;
	    FILE *out;

	    //check parameters
	    assert(section != NULL && strlen(section));
	    assert(key != NULL && strlen(key));
	    assert(value != NULL);
	    assert(file !=NULL &&strlen(key));

	    if(load_ini_file(file) != 0)
	    {
		    sec_start = -1;
	    }
	    else
	    {
		    parse_file(section,key,m_lpszBuffer,&sec_start,&sec_end,&key_start,&key_end,&value_start,&value_end);
	    }

	    if( -1 == sec_start)
	    {
		    if(0==m_nFileSize)
		    {
			    sprintf(w_buf+m_nFileSize,"[%s]\n%s=%s\n",section,key,value);
		    }
		    else
		    {
			    //not find the section, then add the new section at end of the file
			    memcpy(w_buf,m_lpszBuffer,m_nFileSize);
			    sprintf(w_buf+m_nFileSize,"\n[%s]\n%s=%s\n",section,key,value);
		    }
	    }
	    else if(-1 == key_start)
	    {
		    //not find the key, then add the new key=value at end of the section
		    memcpy(w_buf,m_lpszBuffer,sec_end);
		    sprintf(w_buf+sec_end,"%s=%s\n",key,value);
		    sprintf(w_buf+sec_end+strlen(key)+strlen(value)+2,m_lpszBuffer+sec_end, m_nFileSize - sec_end);
	    }
	    else
	    {
		    //update value with new value
		    memcpy(w_buf,m_lpszBuffer,value_start);
		    memcpy(w_buf+value_start,value, value_len);
		    memcpy(w_buf+value_start+value_len, m_lpszBuffer+value_end, m_nFileSize - value_end);
	    }
	
	    out = fopen(file,"w");
	    if(NULL == out)
	    {
		    return 0;
	    }
	
	    if(-1 == fputs(w_buf,out) )
	    {
		    fclose(out);
		    return 0;
	    }

	    fclose(out);
	    return 1;
    }

	// check if my space char now.
	static int myisspace(int _C)
	{
		if ((_C == ' ') || (_C == '\t'))
			return 1;
		return 0;
	}

    // try to load the special INI file.
    int load_ini_file(const char* file,const char* section=NULL)
    {
        int nStatus = -1;
        m_nFileSize  =  0;
        do
        {
            FILE* fin = NULL;
            if (!file) break;
            fin = fopen(file,"r");
            if (NULL == fin) {
                break;
            }

            // try to get the data.
            char data = fgetc(fin);
            while (data != (char)EOF)
            {
                m_nFileSize++;
                data = fgetc(fin);
            }

            // allocate now.
            if (m_nFileSize)
            {
                FreeBuffer();
                m_lpszBuffer = (char*)malloc(m_nFileSize+32);
                if (!m_lpszBuffer) {
                    nStatus = -1;
                    break;
                }

                // section now.
                if (section) {
                    // try to get the special section name value item.
                    strncpy(m_szSection,section,sizeof(m_szSection));
                }

                int idx=0;
                // seek begin of file.
                fseek(fin,0,SEEK_SET);
                // try to get the data.
                char* buf = m_lpszBuffer;
                buf[idx] = fgetc(fin);
                while (buf[idx] != (char)EOF) {
                    idx++; // add the index.
                    buf[idx] = fgetc(fin);
                }

                // read file.
                buf[idx]='\0';
				// clear all the space.
				std::string str = buf;
				str.erase(remove_if(str.begin(), str.end(), myisspace), str.end());
				memset(buf, 0, m_nFileSize);
				memcpy(buf, str.data(), str.size());
                fclose(fin);
                nStatus=0;
            }
	    /* 
            else
            {
                nStatus=0;
            } */
        } while (0);
    //Cleanup:
        return (nStatus);
    }

protected:
    char m_szFile[MAX_PATH];
    char m_szSection[MAX_PATH];
    char* m_lpszBuffer;
    int m_nFileSize;
};

// inline.
inline int GetPrivateProfileInt(const char* section,const char* keyname,int defval,const char* inifile)
{
	int val = defval;
	do
	{
		CINIHelp help;
		if (help.Open(inifile,section)) {
			val = help.GetLong(keyname);
		}

	} while (0);
//Cleanup:
	return (val);
}

inline int GetPrivateProfileString(const char* section,
		const char* keyname,const char* defval,
		char* outbuf,int bufsize,const char* inifile)
{
	int val=0;
	do
	{
		memset(outbuf,0,bufsize);
		CINIHelp help;
		if (help.Open(inifile,section)) {
			help.GetString(section,keyname,outbuf,bufsize);
			val=lstrlen(outbuf);
		}

	} while (0);
//Cleanup:
	return (val);
}



#endif//__INIHELP_HEADER__

           
