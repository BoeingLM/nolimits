#include "main.hpp"

float g_fTickrate = 0.0f;
int g_iMaxClients = 0;

void * pCGlobalVars;
void * pCGameServer;

VTFHook* GetTickInterval;
VTFHook* FindParam;

ICommandLine* pCommandLine;
int GAME = -1;

typedef void *(*FakeGetCommandLine)();

CSamplePlugin g_SamplePlugin;

InterfaceReg *InterfaceReg::s_pInterfaceRegs = nullptr;

InterfaceReg::InterfaceReg( InstantiateInterfaceFn fn , const char *pName ) : m_pName( pName )
{
    m_CreateFn = fn;
    m_pNext = s_pInterfaceRegs;
    s_pInterfaceRegs = this;
}

void* CreateInterfaceInternal( const char *pName , int *pReturnCode )
{
    InterfaceReg *pCur;

    for ( pCur = InterfaceReg::s_pInterfaceRegs; pCur; pCur = pCur->m_pNext )
    {
        if ( strcmp( pCur->m_pName , pName ) == 0 )
        {
            if ( pReturnCode )
            {
                *pReturnCode = IFACE_OK;
            }
            return pCur->m_CreateFn();
        }
    }

    if ( pReturnCode )
        *pReturnCode = IFACE_FAILED;

    return NULL;
}

DLL_EXPORT void* CreateInterface( const char *pName , int *pReturnCode ){
    return CreateInterfaceInternal( pName , pReturnCode );
}

float OnGetTickInterval()
{
	if(g_iMaxClients)
	{
		if(GAME == CSS90)
		{
			if(pCGlobalVars)
				*((int*)pCGlobalVars+20) = g_iMaxClients;
			if(pCGameServer)
			{
				*((int*)pCGameServer + 124) = g_iMaxClients; //maxplayers set to %i\n
				*((int*)pCGameServer + 83) = g_iMaxClients;
			}
		}
		else if(GAME == CSS34)
		{
			if(pCGlobalVars)
				*((int*)pCGlobalVars+20) = g_iMaxClients; 
			if(pCGameServer)
			{
				*((int*)pCGameServer + 64) = g_iMaxClients; //maxplayers set to %i\n		
				*((int*)pCGameServer + 105) = g_iMaxClients;		
			}
		}
	}
	return g_fTickrate;
}

bool CSamplePlugin::Load( CreateInterfaceFn interfaceFactory , CreateInterfaceFn gameServerFactory )
{
	//interfaces ServerGameDLL
	const char strings[3][32] = {
		"ServerGameDLL010", 
		"ServerGameDLL006", 
		"ServerGameDLL005"
		};

	//identify game
	void* server;
	for(unsigned int i = 0; i < sizeof(strings); i++)
	{
		if( (server = (void*)gameServerFactory(strings[i], NULL)) )
		{
			GAME = i;
			break;
		}
	}

	if(!server)
		return false;
	
	void *pEngineServer = (void*)interfaceFactory(GAME != CSS34 ? "VEngineServer023":"VEngineServer021", NULL); //CVEngineServer : public IVEngineServer
	
	if(!pEngineServer)
		return false;
	
	char buffer[256];
	
	switch(GAME)
	{
		case CSS90: strcpy(buffer,"libtier0.so"); break;
		case CSS34: strcpy(buffer,"tier0_i486.so"); break; 
		case CSGO:  strcpy(buffer,"libtier0.so"); break;
	}

	void *tier0 = dlopen(buffer, RTLD_NOW);
	if(!tier0)
		return false;
	void * lb = dlsym(tier0,GAME == CSGO ? "CommandLine":"CommandLine_Tier0");
	if(!lb)
		return false;
	pCommandLine = (ICommandLine*)((FakeGetCommandLine)((FakeGetCommandLine *)lb))();

	switch(GAME)
	{
		case CSS90: callvfunc<void(*)(void*,char*,int)>(pEngineServer, 51)(pEngineServer,buffer,sizeof(buffer)); break; //GetGameDir(buffer, sizeof(buffer));
		case CSS34: callvfunc<void(*)(void*,char*,int)>(pEngineServer, 54)(pEngineServer,buffer,sizeof(buffer)); break; 
		case CSGO:  callvfunc<void(*)(void*,char*,int)>(pEngineServer, 52)(pEngineServer,buffer,sizeof(buffer)); break; //find text "COM_CheckGameDirectory: game directories don't match" or "asw_build_map %s connecting\n"
	}
	
	
	//load config
	strcat(buffer, "/addons/nolimits.ini");
	FILE *fp = fopen(buffer, "r");
	if (fp != NULL)
	{
		if(fgets(buffer,4,fp)) 
			g_fTickrate = atof(buffer);
		
		unsigned int i;

		size_t l;
		while(fgets(buffer,32,fp) != NULL && buffer[0])
		{
			//trim newline character
			l = strlen(buffer) - 1;
			if (buffer[l] == '\n') 
				buffer[l--] = 0;
			if (buffer[l] == '\r') 
				buffer[l] = 0;
			
			if(buffer[0] == '#')
			{
				if(GAME == CSGO)
					pCommandLine->RemoveParamCSGO(&buffer[1]);
				else
					pCommandLine->RemoveParamCSS(&buffer[1]);
				continue;
			}
			for(i = 0;i < strlen(buffer);i++)
			{
				if(buffer[i] == ' ')
				{
					buffer[i] = 0;
					if(GAME == CSGO)
						pCommandLine->AppendParamCSGO(buffer,&buffer[i+1]);	
					else
					{
						if(!strcmp(buffer,"-maxplayers"))
							g_iMaxClients = atoi(&buffer[i+1]);
						pCommandLine->AppendParamCSS(buffer,&buffer[i+1]);	
					}
					break;
				}
			}
		}
		
		if (g_fTickrate > 10)
			g_fTickrate = 1.0f / g_fTickrate;

		//change function in vtable
		if(g_fTickrate)
			GetTickInterval = new VTFHook(server,(void*)OnGetTickInterval,GAME == CSGO ? 9:10);
		
		if(GAME != CSGO)
		{
			//valid interface on all games
			void * pIPlayerInfoManager = (void*)gameServerFactory("PlayerInfoManager002",NULL);
			
			//valid index on all games
			pCGlobalVars = callvfunc<void*(*)(void*)>(pIPlayerInfoManager, 1)(pIPlayerInfoManager);
			
			if(GAME == CSS90)
				pCGameServer = callvfunc<void*(*)(void*)>(pEngineServer, 119)(pEngineServer); //GetIServer
			else if(GAME == CSS34)
			{
				void **pServerVMT = *(void***)server; 
				pCGameServer = *(void**)((uintptr_t)pServerVMT[39]+0x16); //LightStyle
			}
		}
	
		return true;
	}

	return false;
}

void CSamplePlugin::Unload()
{
	if(GetTickInterval)
		delete GetTickInterval;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CSamplePlugin , IServerPluginCallbacks , INTERFACEVERSION_ISERVERPLUGINCALLBACKS , g_SamplePlugin );