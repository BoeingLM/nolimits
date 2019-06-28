#ifndef MAIN_DLL
#define MAIN_DLL

#define FORCEINLINE inline

#if __x86_64__ || __ppc64__
#define MX64
#else
#define MX86
#endif

//for work on all games
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS				"ISERVERPLUGINCALLBACKS002"
#define IFACE_PTR(x) (( int* )*( int* )x)

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include "vtablehook.h"

template <typename Fn = void*>
Fn callvfunc(void* class_base, size_t index){
	return (*reinterpret_cast<Fn**>(class_base))[index];
}

enum
{
    IFACE_OK = 0 ,
    IFACE_FAILED
};

typedef enum
{
    PLUGIN_CONTINUE = 0 , // keep going
    PLUGIN_OVERRIDE , // run the game dll function but use our return value instead
    PLUGIN_STOP , // don't run the game dll function at all
} PLUGIN_RESULT;


typedef enum
{
    eQueryCvarValueStatus_ValueIntact = 0 ,	// It got the value fine.
    eQueryCvarValueStatus_CvarNotFound = 1 ,
    eQueryCvarValueStatus_NotACvar = 2 ,		// There's a ConCommand, but it's not a ConVar.
    eQueryCvarValueStatus_CvarProtected = 3	// The cvar was marked with FCVAR_SERVER_CAN_NOT_QUERY, so the server is not allowed to have its value.
} EQueryCvarValueStatus;


class CCommand;
struct edict_t;
class KeyValues;

typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t byte;
typedef void* Pointer;
typedef unsigned char BYTE;
typedef int QueryCvarCookie_t;

enum
{
	CSS90 = 0,
	CSS34,
	CSGO
};

typedef void* ( *CreateInterfaceFn )( const char *pName , int *pReturnCode );
typedef void* ( *InstantiateInterfaceFn )( );

#define  DLL_EXPORT   extern "C" __attribute__ ((visibility("default")))
#define  DLL_IMPORT   extern "C"

#define  DLL_CLASS_EXPORT __attribute__ ((visibility("default")))
#define  DLL_CLASS_IMPORT

#define  DLL_GLOBAL_EXPORT   extern __attribute ((visibility("default")))
#define  DLL_GLOBAL_IMPORT   extern

#define  DLL_LOCAL __attribute__ ((visibility("hidden")))

#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	static InterfaceReg __g_Create##interfaceName##_reg(functionName, versionName);
#else
#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static InterfaceReg __g_Create##interfaceName##_reg(functionName, versionName); \
	}
#endif

#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	static void* __Create##className##_interface() {return static_cast<interfaceName *>( new className );} \
	static InterfaceReg __g_Create##className##_reg(__Create##className##_interface, versionName );
#else
#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static void* __Create##className##_interface() {return static_cast<interfaceName *>( new className );} \
		static InterfaceReg __g_Create##className##_reg(__Create##className##_interface, versionName ); \
	}
#endif

// Use this to expose a singleton interface with a global variable you've created.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR_WITH_NAMESPACE(className, interfaceNamespace, interfaceName, versionName, globalVarName) \
	static void* __Create##className##interfaceName##_interface() {return static_cast<interfaceNamespace interfaceName *>( &globalVarName );} \
	static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName);
#else
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR_WITH_NAMESPACE(className, interfaceNamespace, interfaceName, versionName, globalVarName) \
	namespace _SUBSYSTEM \
	{ \
		static void* __Create##className##interfaceName##_interface() {return static_cast<interfaceNamespace interfaceName *>( &globalVarName );} \
		static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName); \
	}
#endif

#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName) \
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR_WITH_NAMESPACE(className, , interfaceName, versionName, globalVarName)

// Use this to expose a singleton interface. This creates the global variable for you automatically.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	static className __g_##className##_singleton; \
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, __g_##className##_singleton)
#else
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static className __g_##className##_singleton; \
	}	\
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, __g_##className##_singleton)
#endif

class ICommandLine
{
public:
	const char* GetCmdLine(){
		return callvfunc<const char*(*)(void*)>(this, 2)(this);
	}
	void RemoveParamCSS(const char *param){
		callvfunc<void(*)(void*,const char*)>(this, 4)(this,param);
	}
	void RemoveParamCSGO(const char *param){
		callvfunc<void(*)(void*,const char*)>(this, 5)(this,param);
	}
	void AppendParamCSS(const char *pszParam, const char *pszValues){
		callvfunc<void(*)(void*,const char*,const char*)>(this, 5)(this,pszParam,pszValues);
	}
	void AppendParamCSGO(const char *pszParam, const char *pszValues){
		callvfunc<void(*)(void*,const char*,const char*)>(this, 6)(this,pszParam,pszValues);
	}
};


class InterfaceReg
{
public:
    InterfaceReg( InstantiateInterfaceFn fn , const char *pName );

public:
    InstantiateInterfaceFn	m_CreateFn;
    const char				*m_pName;

    InterfaceReg			*m_pNext; // For the global list.
    static InterfaceReg		*s_pInterfaceRegs;
};


class IServerPluginCallbacks
{
public:
    virtual bool			Load( CreateInterfaceFn interfaceFactory , CreateInterfaceFn gameServerFactory ) = 0;
	virtual void			Unload ( void ) = 0;
	virtual void			Pause( void ) = 0;
    virtual void			UnPause( void ) = 0;
    virtual const char     *GetPluginDescription( void ) = 0;
    virtual void			LevelInit( char const *pMapName ) = 0;
    virtual void			ServerActivate( edict_t *pEdictList , int edictCount , int clientMax ) = 0;
    virtual void			GameFrame( bool simulating ) = 0;
    virtual void			LevelShutdown( void ) = 0;
    virtual void			ClientActive( edict_t *pEntity ) = 0;
    virtual void			ClientFullyConnect( edict_t *pEntity ) = 0;
    virtual void			ClientDisconnect( edict_t *pEntity ) = 0;
    virtual void			ClientPutInServer( edict_t *pEntity , char const *playername ) = 0;
    virtual void			SetCommandClient( int index ) = 0;
    virtual void			ClientSettingsChanged( edict_t *pEdict ) = 0;
    virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect , edict_t *pEntity , const char *pszName , const char *pszAddress , char *reject , int maxrejectlen ) = 0;
    virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity , const CCommand &args ) = 0;
    virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName , const char *pszNetworkID ) = 0;
    virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie , edict_t *pPlayerEntity , EQueryCvarValueStatus eStatus , const char *pCvarName , const char *pCvarValue ) = 0;
    virtual void			OnEdictAllocated( edict_t *edict ) = 0;
    virtual void			OnEdictFreed( const edict_t *edict ) = 0;
};

class CSamplePlugin : public IServerPluginCallbacks 
{
public:
	CSamplePlugin(){
        m_iClientCommandIndex = 0;
    }
    ~CSamplePlugin(){
    }
	
    virtual bool Load( CreateInterfaceFn interfaceFactory , CreateInterfaceFn gameServerFactory );
	virtual void Unload ( void );
	virtual void Pause( void ){ };
    virtual void UnPause( void ){ };
    virtual const char *GetPluginDescription( void ) {
        return "NoLimits v1.0 by vk.com/whereiamnow aka Samoletik1337";
    };
	virtual void LevelInit( char const *pMapName ) { };
    virtual void ServerActivate( edict_t *pEdictList , int edictCount , int clientMax ) { };
    virtual void GameFrame( bool simulating ) { };
    virtual void LevelShutdown( void ) { };
    virtual void ClientActive( edict_t *pEntity ) { };
    virtual void ClientFullyConnect( edict_t *pEntity ) { };
    virtual void ClientDisconnect( edict_t *pEntity ) { };
    virtual void ClientPutInServer( edict_t *pEntity , char const *playername ) { };
    virtual void SetCommandClient( int index ) { };
    virtual void ClientSettingsChanged( edict_t *pEdict ) { };
    virtual PLUGIN_RESULT ClientConnect( bool *bAllowConnect , edict_t *pEntity , const char *pszName , const char *pszAddress , char *reject , int maxrejectlen ){ return PLUGIN_CONTINUE; };
    virtual PLUGIN_RESULT ClientCommand( edict_t *pEntity , const CCommand &args ){ return PLUGIN_CONTINUE; };
    virtual PLUGIN_RESULT NetworkIDValidated( const char *pszUserName , const char *pszNetworkID ){ return PLUGIN_CONTINUE; };
    virtual void OnQueryCvarValueFinished( QueryCvarCookie_t iCookie , edict_t *pPlayerEntity , EQueryCvarValueStatus eStatus , const char *pCvarName , const char *pCvarValue ) { }; 
    virtual void OnEdictAllocated( edict_t *edict ) { };
    virtual void OnEdictFreed( const edict_t *edict ) { };
    virtual void FireGameEvent( KeyValues *event ) { };
	int GetCommandIndex(){
        return m_iClientCommandIndex;
    }

private:
    int m_iClientCommandIndex;
};

#endif
