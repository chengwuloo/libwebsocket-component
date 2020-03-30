

#ifndef __PATHMAGIC_HEADER__
#define __PATHMAGIC_HEADER__

#include "types.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wordexp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


// class path magic.
class CPathMagic
{
public:
    /// format string.
    static LPCTSTR Format( LPCTSTR lpsz, ... )
    {
        va_list va;
        static TCHAR szTemp[4096] = TEXT("");
        // output the buffer.
        va_start(va, lpsz);
        vsnprintf(szTemp, sizeof(szTemp), szTemp, va);
        va_end(va);
    //Cleanup:
        return szTemp;
    }

    /// expand an path.
    static LPCTSTR Expand(LPCTSTR lpsz)
    {
	wordexp_t  p;
	char** w = 0;
        static TCHAR szTemp[4096] = TEXT("");
	// expand the environment string now.
	wordexp(lpsz,&p,0);
	if (p.we_wordv)
	{
		LPTSTR pos = szTemp;
		for (unsigned int i=0;i<p.we_wordc;i++)
		{
			strcat(pos,w[i]);
			pos += strlen(szTemp);
		}
		
		// free the word.
		wordfree(&p);
		return szTemp;
	}
    //Cleanup:
        return NULL;
    }

    /// get the current module path.
    static LPCTSTR Current( HINSTANCE hInst = NULL )
    {
        static TCHAR szMod[MAX_PATH] = TEXT("");
        if ( !GetModuleFileName( hInst, szMod, MAX_PATH ))
            return NULL;

        // remove the file name special.
        PathRemoveFileSpec( szMod );
        return szMod;
    }

    /// get the path for special pathname item value.
    static LPCWSTR Path4String(LPCTSTR lpszPathname)
    {
        static TCHAR szMod[MAX_PATH] = {0};
        // get the content of pathname.
        strncpy(szMod, lpszPathname, MAX_PATH);
        // remove the file name special.
        PathRemoveFileSpec(szMod);
        return szMod;
    }

    /// build the full path name.
    static LPCTSTR Build( LPCTSTR lpszPath, LPCTSTR lpszModule )
    {
        static TCHAR szMod[MAX_PATH+1] = TEXT("");

        // check the path and filename.
        if (( !lpszPath ) || ( !lpszModule ))
            return NULL;

        // build the full path name.
        strncpy(szMod, lpszPath, MAX_PATH+1);
        PathAppend(szMod, lpszModule);
        return szMod;
    }

    /// get the filename of pathname item value.
    static LPTSTR Filename(LPTSTR lpszPath)
    {
        LPCTSTR lpsz = NULL;
        static TCHAR lpszFileName[MAX_PATH] = TEXT("");
        lpsz = PathFindFileName(lpszPath);
        // try to get the full pathname.
        strncpy(lpszFileName, lpsz, MAX_PATH);
        return (lpszFileName);
    }

	/// get the filename of pathname content now.
	static LPCTSTR FileBaseName(LPCTSTR lpszPath)
	{
		LPCTSTR lpsz = NULL;
		static TCHAR szFileName[MAX_PATH] = {0};
		lpsz = PathFindFileName( lpszPath );
		// try to get the full pathname.
		strncpy(szFileName, lpsz, MAX_PATH);
		PathRemoveExtension(szFileName);
		return (szFileName);
	}

	//
    // get the special name for current module.
    //
	static LPCTSTR Path4Module( LPCTSTR lpszModule, HINSTANCE hInst = NULL )
    {
        LPCTSTR        lpszCurr = NULL;
        static LPCTSTR lpszFull = NULL;

        // try to get the path of current module.
        lpszCurr = Current( hInst );
        if ( !lpszCurr )
            return NULL;

        // try to full the path name
        lpszFull = Build( lpszCurr, lpszModule );
        return lpszFull;
    }

	//
    // get the path extension.
    //
	static LPCTSTR PathExtension( LPCTSTR lpszPath )
    {
        static LPCTSTR lpsz = NULL;
        lpsz = PathFindExtension( lpszPath );
        return lpsz;
    }

    // ==================================================================
    //             G E T   W I N D O W S   D I R E C T O R Y 
    // ==================================================================
    /// get the windows directory
    static LPCTSTR WindowsDir()
    {
	return "/usr/local/bin";
    }

    // get the windows system directory
    static LPCTSTR SystemDir()
    {
	return "/usr/local/bin";
    }
    
    // get linux temp directory
    static LPCTSTR TempPath()
    {
	return "/tmp";
    }

    /// get the special path value content now.
    static LPCSTR GetPath(LPCSTR lpszFileName)
    {
	static CHAR szPath[MAX_PATH] = {0};
        // try to get the filename.
        strncpy(szPath, lpszFileName, MAX_PATH);
        // try to remove the file special.
        if (PathRemoveFileSpec(szPath))
        {
            return szPath;
        }
        // failed.
        return NULL;
    }
    
    /// try to get the special module file name content.
    static LPCTSTR GetModuleName(HINSTANCE hInst=NULL)
    {
        LONG    nIndex    = 0;
        LONG    nCount    = 0;
        LPTSTR  lpszSplit = NULL;
        LPTSTR  lpszFile  = NULL;
        TCHAR   szPath[MAX_PATH]={0};
        // try to build the buffer now.
        GetModuleFileName(hInst,szPath,MAX_PATH);
        // try to get filename from module path.
        lpszFile = Filename(szPath);
        // try to remove the extension name.
        if (!lpszFile)
        {
            return (lpszFile);
        }

        // try to find the extension name.
        lpszSplit = lpszFile;
        nCount    = strlen(lpszFile);
        // try to find the index.
        for (nIndex=0;nIndex<nCount;nIndex++)
        {
            // check if the "." flag.
            if ('.'==lpszSplit[nIndex])
            {
                // try to end the string.
                lpszSplit[nIndex]=0;
                // succeeded.
                break;
            }
        }
    //Cleanup:
        return (lpszFile);
    }

public://getcwd
	static LPCTSTR GetModuleFileName(HINSTANCE hInst,TCHAR* buf,int bufsize)
	{
		memset(buf,0,bufsize);
		int cnt = readlink("/proc/self/exe",buf,bufsize);
		if ((cnt<0)||(cnt>MAX_PATH)) {
			return NULL;
		}
	//Cleanup:
		return (buf);
	}

	// try to get the current directory content item data.
	static int GetCurrentDirectory(TCHAR* buf, int bufsize)
	{
		int len = 0;
		char temp[MAX_PATH]={0};
		if (GetModuleFileName(NULL,temp,bufsize))
		{
			len = strlen(temp);
			if (len)
			{
				LPTSTR lst = temp + len - 1;
				// check the special length of content.
				while ((len-- > 0) && (*lst != '/')) {
					lst--;
				}

				// check if find the special flags.
				if (len > 0) *(lst+1) = '\0';
			}
		}

		// try to get the content.
		lstrcpyn(buf,temp,bufsize);
		return len;
	}

	// try to remove the special file path item now.
	static LPTSTR PathRemoveFileSpec(LPTSTR path)
	{
    		LPTSTR lpsz = path;
    		do
   		{
        		int len = strlen(path);
        		if (len<=0) {
            			break;
        		}

        		/// update the special path.
        		LPTSTR lst = path + len - 1;
        		while ((len-- > 0) && (*lst != '/')) {
            			lst--;   
        		};

        		// update the last char now.
        		if (len > 0) *(lst+1) = '\0';
    		}   while (0);
	//Cleanup:
    		return (lpsz);
	}

	// try to append the special content value now.
	static LPCTSTR PathFindExtension(LPCTSTR path)
	{
    		LPCTSTR lpsz = NULL;
    		do
    		{
        		int len = strlen(path);
        		if (len<=0) {
            			break;
        		}

        		/// update the special path.
        		LPCTSTR lst = path + len - 1;
        		while ((len-- > 0) && (*lst != '.')) {
            			lst--;   
        		};

        		// get name now.
        		if (len > 0) {
            			lpsz = lst + 1;
       			}
    		}   while (0);
	//Cleanup:
    		return (lpsz);
	}

	// try to append the special content value now.
	static LPTSTR PathRemoveExtension(LPTSTR path)
	{
    		LPTSTR lpsz = path;
    		do
    		{
        		int len = strlen(path);
        		if (len<=0) {
            			break;
        		}

        		/// update the special path.
        		LPTSTR lst = path + len - 1;
        		while ((len-- > 0) && (*lst != '.')) {
            			lst--;   
        		};

        		// get name now.
        		if (len > 0) {
            			*lst = '\0';
        		}
    		}   while (0);
	//Cleanup:
    		return (lpsz);
	}
	
	// try to append the special content value item value data now.
	static LPTSTR PathRenameExtension(LPTSTR path,LPCTSTR extname)
	{
    		LPTSTR lpsz = path;
    		do
    		{
        		int len = strlen(path);
        		if (len<=0) {
            			break;
        		}

        		/// update the special path.
        		LPTSTR lst = path + len - 1;
        		while ((len-- > 0) && (*lst != '.')) {
            			lst--;   
        		};

        		// get name now.
        		if (len > 0) {
            			*lst = '\0';
						strcat(path,extname);
        		}
				else
				{
					// append the path with extension name.
					strcat(path,extname);
				}

    		}   while (0);
	//Cleanup:
    		return (lpsz);
	}

	// try to append the special content value item now.
	static LPTSTR PathAppend(LPTSTR path,LPCTSTR file)
	{
    		LPTSTR lpsz = path;
    		do 
    		{
        		int len = strlen(path);
        		if (len<=0) {
            			break;
        		}

        		// check the last char item now.
        		LPTSTR tmp = (path + len - 1);
        		if (*tmp  !='/') {
            			strcat(tmp,"/");
        		}

        		// append file name.
        		strcat(path,file);
    		} while (0);
	//Cleanup:
    		return (lpsz);
	}	

	// find the special file name from path now.
	static LPCTSTR PathFindFileName(LPCTSTR path)
	{
    		LPCTSTR lpsz = NULL;
    		do
    		{
        		int len = strlen(path);
        		if (len<=0) {
            			break;
        		}

        		/// update the special path.
        		LPCTSTR lst = path + len - 1;
        		while ((len-- > 0) && (*lst != '/')) {
            			lst--;   
        		};

        		// get name now.
        		if (len > 0) {
            			lpsz = (lst + 1);
        		}
    		}   while (0);
	//Cleanup:
    		return (lpsz);
	}

	// check if the sepcial directory has  exist.
	static BOOL PathIsDirectory(const char* path)
	{
		if (!path) return false;
		if (opendir(path) != NULL) {
			return TRUE;
		} 
	//Cleanup:
		return FALSE;
	}

	// try to create the special directory content item data now.
	static BOOL CreateDirectory(LPCTSTR path,LPVOID lpSecInfo=0)
	{
		int status = mkdir(path,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
		if (0 == status) {
			return TRUE;
		}
	//Cleanup:
		return (FALSE);
	}


};



#endif//__PATHMAGIC_HEADER__
