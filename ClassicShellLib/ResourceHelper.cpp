// Classic Shell (c) 2009-2013, Ivo Beltchev
// The sources for Classic Shell are distributed under the MIT open source license

#define STRICT_TYPED_ITEMIDS
#include <windows.h>
#include "StringSet.h"
#include "StringUtils.h"
#include "Translations.h"
#include "ResourceHelper.h"
#include <shlobj.h>
#include <vector>
#include <wincodec.h>

static HINSTANCE g_MainInstance;
static CStringSet g_ResStrings;
static std::map<int,std::vector<char> > g_ResDialogs;

// Loads all strings from hLngInstance
// pDialogs is a NULL-terminated list of dialog IDs. They are loaded from hLngInstance if possible, otherwise from hMainInstance
void LoadTranslationResources( HINSTANCE hMainInstance, HINSTANCE hLngInstance, int *pDialogs )
{
	g_MainInstance=hMainInstance;
	if (hLngInstance)
	{
		LoadTranslationOverrides(hLngInstance);
		g_ResStrings.Init(hLngInstance);
	}
	for (int i=0;pDialogs[i];i++)
	{
		int id=pDialogs[i];
		HINSTANCE hInst=hLngInstance;
		HRSRC hrsrc=NULL;
		if (hLngInstance)
			hrsrc=FindResource(hInst,MAKEINTRESOURCE(id),RT_DIALOG);
		if (!hrsrc)
		{
			hInst=hMainInstance;
			hrsrc=FindResource(hInst,MAKEINTRESOURCE(id),RT_DIALOG);
		}
		if (hrsrc)
		{
			HGLOBAL hglb=LoadResource(hInst,hrsrc);
			if (hglb)
			{
				// finally lock the resource
				LPVOID res=LockResource(hglb);
				std::vector<char> &dlg=g_ResDialogs[id];
				dlg.resize(SizeofResource(hInst,hrsrc));
				if (!dlg.empty())
					memcpy(&dlg[0],res,dlg.size());
			}
		}
	}
}

// Returns a localized string
CString LoadStringEx( int stringID )
{
	CString str=g_ResStrings.GetString(stringID);
	if (str.IsEmpty())
		str.LoadString(g_MainInstance,stringID);
	return str;
}

// Returns a localized dialog template
DLGTEMPLATE *LoadDialogEx( int dlgID )
{
	std::map<int,std::vector<char> >::iterator it=g_ResDialogs.find(dlgID);
	if (it==g_ResDialogs.end())
		return NULL;
	if (it->second.empty())
		return NULL;
	return (DLGTEMPLATE*)&it->second[0];
}

// Loads an icon. path can be a path to .ico file, or in the format "module.dll, number"
HICON LoadIcon( int iconSize, const wchar_t *path, std::vector<HMODULE> &modules )
{
	wchar_t text[1024];
	Strcpy(text,_countof(text),path);
	DoEnvironmentSubst(text,_countof(text));
	wchar_t *c=wcsrchr(text,',');
	if (c)
	{
		// resource file
		*c=0;
		const wchar_t *res=c+1;
		int idx=_wtol(res);
		if (idx>0) res=MAKEINTRESOURCE(idx);
		if (!text[0])
			return (HICON)LoadImage(_AtlBaseModule.GetResourceInstance(),res,IMAGE_ICON,iconSize,iconSize,LR_DEFAULTCOLOR);
		HMODULE hMod=GetModuleHandle(PathFindFileName(text));
		if (!hMod)
		{
			hMod=LoadLibraryEx(text,NULL,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
			if (!hMod) return NULL;
			modules.push_back(hMod);
		}
		return (HICON)LoadImage(hMod,res,IMAGE_ICON,iconSize,iconSize,LR_DEFAULTCOLOR);
	}
	else
	{
		return (HICON)LoadImage(NULL,text,IMAGE_ICON,iconSize,iconSize,LR_DEFAULTCOLOR|LR_LOADFROMFILE);
	}
}

HICON LoadIcon( int iconSize, PIDLIST_ABSOLUTE pidl )
{
	HICON hIcon=NULL;
	CComPtr<IShellFolder> pFolder;
	PCITEMID_CHILD child;
	if (SUCCEEDED(SHBindToParent(pidl,IID_IShellFolder,(void**)&pFolder,&child)))
	{
		bool bLarge=(iconSize>GetSystemMetrics(SM_CXSMICON));
		LONG lSize;
		if (bLarge)
			lSize=MAKELONG(iconSize,GetSystemMetrics(SM_CXSMICON));
		else
			lSize=MAKELONG(GetSystemMetrics(SM_CXICON),iconSize);
		CComPtr<IExtractIcon> pExtract;
		if (SUCCEEDED(pFolder->GetUIObjectOf(NULL,1,&child,IID_IExtractIcon,NULL,(void**)&pExtract)))
		{
			// get the icon location
			wchar_t location[_MAX_PATH];
			int index=0;
			UINT flags=0;
			if (pExtract->GetIconLocation(0,location,_countof(location),&index,&flags)==S_OK)
			{
				if (flags&GIL_NOTFILENAME)
				{
					// extract the icon
					HICON hIcon2=NULL;
					HRESULT hr=pExtract->Extract(location,index,bLarge?&hIcon:&hIcon2,bLarge?&hIcon2:&hIcon,lSize);
					if (FAILED(hr))
						hIcon=hIcon2=NULL;
					if (hr==S_FALSE)
						flags=0;
					if (hIcon2) DestroyIcon(hIcon2); // HACK!!! Even though Extract should support NULL, not all implementations do. For example shfusion.dll crashes
				}
				if (!(flags&GIL_NOTFILENAME))
				{
					if (ExtractIconEx(location,index==-1?0:index,bLarge?&hIcon:NULL,bLarge?NULL:&hIcon,1)!=1)
						hIcon=NULL;
				}
			}
		}
		else
		{
			// try again using the ANSI version
			CComPtr<IExtractIconA> pExtractA;
			if (SUCCEEDED(pFolder->GetUIObjectOf(NULL,1,&child,IID_IExtractIconA,NULL,(void**)&pExtractA)))
			{
				// get the icon location
				char location[_MAX_PATH];
				int index=0;
				UINT flags=0;
				if (pExtractA->GetIconLocation(0,location,_countof(location),&index,&flags)==S_OK)
				{
					if (flags&GIL_NOTFILENAME)
					{
						// extract the icon
						HICON hIcon2=NULL;
						HRESULT hr=pExtractA->Extract(location,index,bLarge?&hIcon:&hIcon2,bLarge?&hIcon2:&hIcon,lSize);
						if (FAILED(hr))
							hIcon=hIcon2=NULL;
						if (hr==S_FALSE)
							flags=0;
						if (hIcon2) DestroyIcon(hIcon2); // HACK!!! Even though Extract should support NULL, not all implementations do. For example shfusion.dll crashes
					}
					if (!(flags&GIL_NOTFILENAME))
					{
						if (ExtractIconExA(location,index==-1?0:index,bLarge?&hIcon:NULL,bLarge?NULL:&hIcon,1)!=1)
							hIcon=NULL;
					}
				}
			}
		}
	}

	return hIcon;
}

HICON ShExtractIcon( const wchar_t *path, int index, int iconSize )
{
	HICON hIcon;

	typedef UINT (WINAPI *FSHExtractIconsW)( LPCWSTR pszFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phIcon, UINT *pIconId, UINT nIcons, UINT flags );
	static FSHExtractIconsW s_SHExtractIconsW;

	if (!s_SHExtractIconsW)
	{
		HMODULE hShell32=GetModuleHandle(L"Shell32.dll");
		if (hShell32)
			s_SHExtractIconsW=(FSHExtractIconsW)GetProcAddress(hShell32,"SHExtractIconsW");
	}

	if (s_SHExtractIconsW)
	{
		UINT id;
		if (!s_SHExtractIconsW(path,index,iconSize,iconSize,&hIcon,&id,1,LR_DEFAULTCOLOR))
			hIcon=NULL;
	}
	else
	{
		if (ExtractIconEx(path,index,&hIcon,NULL,1)!=1)
			return NULL;
	}
	return hIcon;
}

HICON ShExtractIcon( const char *path, int index, int iconSize )
{
	wchar_t pathW[_MAX_PATH];
	MbsToWcs(pathW,_countof(pathW),path);
	return ShExtractIcon(pathW,index,iconSize);
}

// Converts an icon to a bitmap. pBits may be NULL. If bDestroyIcon is true, hIcon will be destroyed
HBITMAP BitmapFromIcon( HICON hIcon, int iconSize, unsigned int **pBits, bool bDestroyIcon )
{
	BITMAPINFO bi={0};
	bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth=bi.bmiHeader.biHeight=iconSize;
	bi.bmiHeader.biPlanes=1;
	bi.bmiHeader.biBitCount=32;
	RECT rc={0,0,iconSize,iconSize};

	HDC hdc=CreateCompatibleDC(NULL);
	unsigned int *bits;
	HBITMAP bmp=CreateDIBSection(hdc,&bi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	HGDIOBJ bmp0=SelectObject(hdc,bmp);
	FillRect(hdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
	DrawIconEx(hdc,0,0,hIcon,iconSize,iconSize,0,NULL,DI_NORMAL);
	SelectObject(hdc,bmp0);
	DeleteDC(hdc);
	if (bDestroyIcon) DestroyIcon(hIcon);
	if (pBits) *pBits=bits;
	return bmp;
}

// Premultiplies a DIB section by the alpha channel and a given color
void PremultiplyBitmap( HBITMAP hBitmap, COLORREF rgb )
{
	BITMAP info;
	GetObject(hBitmap,sizeof(info),&info);
	int n=info.bmWidth*info.bmHeight;
	int mr=(rgb)&255;
	int mg=(rgb>>8)&255;
	int mb=(rgb>>16)&255;
	// pre-multiply the alpha
	for (int i=0;i<n;i++)
	{
		unsigned int &pixel=((unsigned int*)info.bmBits)[i];
		int a=(pixel>>24);
		int r=(pixel>>16)&255;
		int g=(pixel>>8)&255;
		int b=(pixel)&255;
		r=(r*a*mr)/(255*255);
		g=(g*a*mg)/(255*255);
		b=(b*a*mb)/(255*255);
		pixel=(a<<24)|(r<<16)|(g<<8)|b;
	}
}

// Creates a grayscale version of an icon
HICON CreateDisabledIcon( HICON hIcon, int iconSize )
{
	// convert normal icon to grayscale
	ICONINFO info;
	GetIconInfo(hIcon,&info);

	unsigned int *bits;
	HBITMAP bmp=BitmapFromIcon(hIcon,iconSize,&bits,false);

	int n=iconSize*iconSize;
	for (int i=0;i<n;i++)
	{
		unsigned int &pixel=bits[i];
		int r=(pixel&255);
		int g=((pixel>>8)&255);
		int b=((pixel>>16)&255);
		int l=(77*r+151*g+28*b)/256;
		pixel=(pixel&0xFF000000)|(l*0x010101);
	}

	if (info.hbmColor) DeleteObject(info.hbmColor);
	info.hbmColor=bmp;
	hIcon=CreateIconIndirect(&info);
	DeleteObject(bmp);
	if (info.hbmMask) DeleteObject(info.hbmMask);
	return hIcon;
}

// Loads an image file into a bitmap and optionally resizes it
HBITMAP LoadImageFile( const wchar_t *path, const SIZE *pSize, bool bPremultiply )
{
	HBITMAP srcBmp=NULL;
	if (_wcsicmp(PathFindExtension(path),L".bmp")==0)
	{
		srcBmp=(HBITMAP)LoadImage(NULL,path,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_LOADFROMFILE);
	}
	if (srcBmp && !pSize)
		return srcBmp;
	CComPtr<IWICImagingFactory> pFactory;
	if (FAILED(pFactory.CoCreateInstance(CLSID_WICImagingFactory)))
	{
		if (srcBmp) DeleteObject(srcBmp);
		return NULL;
	}

	CComPtr<IWICBitmapSource> pBitmap;
	if (srcBmp)
	{
		CComPtr<IWICBitmap> pBitmap2;
		if (FAILED(pFactory->CreateBitmapFromHBITMAP(srcBmp,NULL,WICBitmapUseAlpha,&pBitmap2)))
		{
			DeleteObject(srcBmp);
			return NULL;
		}
		pBitmap=pBitmap2;
		DeleteObject(srcBmp);
	}
	else
	{
		CComPtr<IWICBitmapDecoder> pDecoder;
		if (FAILED(pFactory->CreateDecoderFromFilename(path,NULL,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&pDecoder)))
			return NULL;

		CComPtr<IWICBitmapFrameDecode> pFrame;
		if (FAILED(pDecoder->GetFrame(0,&pFrame)))
			return NULL;
		pBitmap=pFrame;
	}

	if (pSize && pSize->cx)
	{
		CComPtr<IWICBitmapScaler> pScaler;
		if (FAILED(pFactory->CreateBitmapScaler(&pScaler)))
			return NULL;
		if (pSize->cy)
			pScaler->Initialize(pBitmap,pSize->cx,pSize->cy,WICBitmapInterpolationModeCubic);
		else
		{
			UINT width=0, height=0;
			pBitmap->GetSize(&width,&height);
			pScaler->Initialize(pBitmap,pSize->cx,pSize->cx*height/width,WICBitmapInterpolationModeFant);
		}
		pBitmap=pScaler;
	}

	CComPtr<IWICFormatConverter> pConverter;
	if (FAILED(pFactory->CreateFormatConverter(&pConverter)))
		return NULL;
	pConverter->Initialize(pBitmap,bPremultiply?GUID_WICPixelFormat32bppPBGRA:GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,NULL,0,WICBitmapPaletteTypeMedianCut);
	pBitmap=pConverter;

	UINT width=0, height=0;
	pBitmap->GetSize(&width,&height);

	BITMAPINFO bi={0};
	bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth=width;
	bi.bmiHeader.biHeight=-(int)height;
	bi.bmiHeader.biPlanes=1;
	bi.bmiHeader.biBitCount=32;

	HDC hdc=CreateCompatibleDC(NULL);
	BYTE *bits;
	HBITMAP bmp=CreateDIBSection(hdc,&bi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	DeleteDC(hdc);

	if (SUCCEEDED(pBitmap->CopyPixels(NULL,width*4,width*height*4,bits)))
		return bmp;

	DeleteObject(bmp);
	return NULL;
}


// Returns the version of a given module
DWORD GetVersionEx( HINSTANCE hInstance )
{
	// get the DLL version. this is a bit hacky. the standard way is to use GetFileVersionInfo and such API.
	// but it takes a file name instead of module handle so it will probably load the DLL a second time.
	// the header of the version resource is a fixed size so we can count on VS_FIXEDFILEINFO to always
	// be at offset 40
	HRSRC hResInfo=FindResource(hInstance,MAKEINTRESOURCE(VS_VERSION_INFO),RT_VERSION);
	if (!hResInfo)
		return 0;
	HGLOBAL hRes=LoadResource(hInstance,hResInfo);
	void *pRes=LockResource(hRes);
	if (!pRes) return 0;

	VS_FIXEDFILEINFO *pVer=(VS_FIXEDFILEINFO*)((char*)pRes+40);
	return ((HIWORD(pVer->dwProductVersionMS)&255)<<24)|((LOWORD(pVer->dwProductVersionMS)&255)<<16)|HIWORD(pVer->dwProductVersionLS);
}

// Returns the Windows version - 0x600, 0x601, ...
WORD GetWinVersion( void )
{
	static WORD version;
	if (!version)
	{
		DWORD ver=GetVersion();
		version=MAKEWORD(HIBYTE(ver),LOBYTE(ver));
	}
	return version;
}

// Wrapper for IShellFolder::ParseDisplayName
HRESULT ShParseDisplayName( wchar_t *pszName, PIDLIST_ABSOLUTE *ppidl, SFGAOF sfgaoIn, SFGAOF *psfgaoOut )
{
	static ITEMIDLIST ilRoot={0};
	static CComPtr<IShellFolder> pDesktop;
	if (_wcsicmp(pszName,L"::{Desktop}")==0)
	{
		*ppidl=ILCloneFull((PIDLIST_ABSOLUTE)&ilRoot);
		if (psfgaoOut)
		{
			*psfgaoOut=0;
			if (sfgaoIn&SFGAO_FOLDER)
				*psfgaoOut|=SFGAO_FOLDER;
		}
		return S_OK;
	}
	else
	{
		*ppidl=NULL;
		if (!pDesktop)
		{
			HRESULT hr=SHGetDesktopFolder(&pDesktop);
			if (FAILED(hr))
				return hr;
		}
		SFGAOF flags=sfgaoIn;
		HRESULT hr=pDesktop->ParseDisplayName(NULL,NULL,pszName,NULL,(PIDLIST_RELATIVE*)ppidl,psfgaoOut?&flags:NULL);
		if (FAILED(hr))
			return hr;
		if (psfgaoOut)
			*psfgaoOut=flags;
		return hr;
	}
}

// Separates the arguments from the program
// May return NULL if no arguments are found
const wchar_t *SeparateArguments( const wchar_t *command, wchar_t *program )
{
	if (command[0]=='"')
	{
		// quoted program - just GetToken will work
		return GetToken(command,program,_MAX_PATH,L" ");
	}

	// skip leading spaces
	while (*command==' ')
		command++;
	const wchar_t *args=wcschr(command,' ');
	if (!args)
	{
		// no spaces - the whole thing is a program
		Strcpy(program,_MAX_PATH,command);
		return NULL;
	}

	int len=(int)(args-command);
	if (len>_MAX_PATH-1) len=_MAX_PATH-1;
	memcpy(program,command,len*2);
	program[len]=0;

	const wchar_t *space=command;
	while (*space)
	{
		space=wcschr(space+1,' ');
		if (!space)
			space=command+Strlen(command);
		len=(int)(space-command);
		if (len>=_MAX_PATH) break;
		wchar_t prog2[_MAX_PATH];
		memcpy(prog2,command,len*2);
		prog2[len]=0;
		if (len>0 && prog2[len-1]=='\\')
			prog2[len-1]=0;
		WIN32_FIND_DATA data;
		HANDLE h=FindFirstFile(prog2,&data);
		if (h!=INVALID_HANDLE_VALUE)
		{
			// found a valid file
			FindClose(h);
			memcpy(program,command,len*2);
			program[len]=0;
			if (*space)
				args=space+1;
			else
				args=NULL;
		}
	}

	while (args && *args==' ')
		args++;
	return args;
}
struct CommonEnvVar
{
	const wchar_t *name;
	wchar_t value[_MAX_PATH];
	int len;
};

CommonEnvVar g_CommonEnvVars[]={
	{L"USERPROFILE"},
	{L"ALLUSERSPROFILE"},
	{L"SystemRoot"},
	{L"SystemDrive"},
};

void UnExpandEnvStrings( const wchar_t *src, wchar_t *dst, int size )
{
	static bool bInit=false;
	if (!bInit)
	{
		bInit=true;
		for (int i=0;i<_countof(g_CommonEnvVars);i++)
		{
			int len=GetEnvironmentVariable(g_CommonEnvVars[i].name,g_CommonEnvVars[i].value,_MAX_PATH);
			if (len<=_MAX_PATH)
				g_CommonEnvVars[i].len=len;
		}
	}

	for (int i=0;i<_countof(g_CommonEnvVars);i++)
	{
		int len=g_CommonEnvVars[i].len;
		if (_wcsnicmp(src,g_CommonEnvVars[i].value,len)==0)
		{
			const wchar_t *name=g_CommonEnvVars[i].name;
			if (Strlen(src)-len+Strlen(name)+3>size)
				break; // not enough space
			Sprintf(dst,size,L"%%%s%%%s",name,src+len);
			return;
		}
	}
	Strcpy(dst,size,src);
}