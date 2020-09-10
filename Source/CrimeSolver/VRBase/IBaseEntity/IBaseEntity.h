#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IBaseEntity_flags.h"
#include "System/Assert.h"
#include "System/EHandle.h"
#include "System/Globals.h"
#include "System/Input.h"
#include "System/predefs.h"
#include "System/static_initialize.h"
#include "UObject/ConstructorHelpers.h"

/**
 *-------------------------------------------------------------------------------------
 * IBaseEntity
 * IBaseEntity is an interface which declares functionality common to all "Base"
 *types and defines some of the most basic functions.
 *-------------------------------------------------------------------------------------
 */
INTERFACE IBaseEntity {
public:
    friend class CGlobalVars; // allow CGlobalVars to access s_iEntityCount
    IBaseEntity();
    virtual ~IBaseEntity() { RemoveSelfFromLists(); }

    ftime m_tConstructionTime;
    void  RemoveSelfFromLists(); // invalidates EHANDLES but DOES NOT modify static
                                 // counts
    bool        DestroyEntity(); // destroys this Actor
    inline void ReportReady() { s_iReadyEntityCount++; }

    //-------------------------------------------------------------------------------------
    // Entity indexing system
    //-------------------------------------------------------------------------------------

    inline eindex EntIndex() const { return m_iEntIndex; }
    EHANDLE       GetEHandle() const { return EHANDLE(this); }

private:
    eindex m_iEntIndex;

protected:
    void        PostDuplicate(EDuplicateMode::Type mode);
    static void AddEntityToLists(IBaseEntity * pEnt);
    static int  s_iReadyEntityCount;
    static int  s_iEntityCount;

    //-------------------------------------------------------------------------------------
    // Initialization system
    //-------------------------------------------------------------------------------------

public:
    virtual void PreInit() {} // called before all the static intializers
    virtual void PostInit();  // called after all the static initializers

    //-------------------------------------------------------------------------------------
    // Linkage to vanilla Unreal system
    //-------------------------------------------------------------------------------------
    // This assumes that this is an actor. If not, segmentation faults
    // may occur, or something worse!

public:
    // bool								IsBaseEntity()					const
    // { return GetActor()->ActorHasTag(TAG_BASEENTITY); }
    bool IsBasePawn() const { return GetActor()->ActorHasTag(TAG_BASEPAWN); }
    bool IsBaseCharacter() const {
        return GetActor()->ActorHasTag(TAG_BASECHARACTER);
    }
    static IBaseEntity* FromActor(AActor * pActor);
    inline IBaseEntity& GetRef() { return *this; }
    inline AActor*      GetActor() const { return m_pSelfAsActor; }

protected:
    AActor* m_pSelfAsActor;

    //-------------------------------------------------------------------------------------
    // Initial transform
    //-------------------------------------------------------------------------------------
protected:
    FTransform m_tInitialTransform;

public:
    FTransform getInitialTransform() { return m_tInitialTransform; };
    FVector    getInitialScale() { return m_tInitialTransform.GetScale3D(); };
    FRotator   getInitialRotation() { return m_tInitialTransform.Rotator(); };
    FVector    getInitialLocation() { return m_tInitialTransform.GetLocation(); };

    //-------------------------------------------------------------------------------------
    // Think System
    //-------------------------------------------------------------------------------------

public:
    typedef void (*BASEPTR)(void*);
    virtual void DefaultThink(); // this think function is always called
    inline void  Think() {
        if (m_pfnThink) m_pfnThink(m_pThinkParam);
    }
    inline void  SetNextThink(ftime time) { m_tNextThink = time; }
    inline ftime GetNextThink() const { return m_tNextThink; }
    inline void  ThinkSet(BASEPTR pProcedure, void* pThinkParam) {
        m_pfnThink    = pProcedure;
        m_pThinkParam = pThinkParam;
    }
    inline void StopThink() {
        m_pfnThink    = NULL;
        m_pThinkParam = NULL;
    }

#define SetThinkEnt(func, ent)                                                      \
    ent->ThinkSet((BASEPTR)(func), reinterpret_cast<void*>(ent))
#define SetThink(func) SetThinkEnt(func, this)

    static inline bool AllEntitiesReady() {
        return s_iReadyEntityCount == s_iEntityCount;
    }

private:
    BASEPTR m_pfnThink    = nullptr;
    void*   m_pThinkParam = nullptr;
    ftime   m_tNextThink;

    //-------------------------------------------------------------------------------------
    // Respawn system
    //-------------------------------------------------------------------------------------

public:
    virtual void Respawn();

private:
    ftime m_tLastRespawn;
    ftime m_tNextRespawn;

    //-------------------------------------------------------------------------------------
    // Health system
    //-------------------------------------------------------------------------------------

public:
    int  GetHealth() const { return m_iHealth; }
    int  GetSpawnHealth() const { return m_iSpawnHealth; }
    void SetHealth(int health);

    inline bool IsInvincible() const { return HasFlags(FL_INVINCIBLE); }
    inline bool IsNotDamageable() const { return HasFlags(FL_NODAMAGE); }

protected:
    inline bool IsAlive() const { return m_iHealth > 0; }
    inline bool IsDead() const { return !IsAlive(); }

protected:
    int m_iSpawnHealth = 100;
    int m_iHealth;
    int m_iDestoryHealth = 0; // health at which this entity is destroyed

    //-------------------------------------------------------------------------------------
    // Flag system
    //-------------------------------------------------------------------------------------
    // compactly stores up to 64 booleans as an unsigned long has 64 bits

public:
    inline ulong GetFlags() const { return m_iFlags; }
    inline bool  HasFlags(ulong flags) const { return (m_iFlags & flags) != 0; }
    inline bool  HasFlagsAll(ulong flags) const {
        return (m_iFlags & flags) == flags;
    }
    inline void AddFlags(ulong flags) { m_iFlags |= flags; }
    inline void RemoveFlags(ulong flags) { m_iFlags &= ~flags; }
    inline void ToggleFlags(ulong flags) { m_iFlags ^= flags; }
    inline void ResetFlags() { m_iFlags = m_iSpawnFlags; }

protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "IBaseEntity")
    ulong m_iSpawnFlags = 0;
    ulong m_iFlags;

    //-------------------------------------------------------------------------------------
    // Generic "use" system
    //-------------------------------------------------------------------------------------

public:
    bool Use(ABaseEntity * pActivator); // returns true on success, false if blocked
    virtual void OnUsed(ABaseEntity * pActivator) {}
    inline bool  IsUseable() const {
        return !HasFlags(FL_IGNORE_USE | FL_INGORE_INPUT);
    }
    virtual bool     IsUseableBy(const ABaseController*) const { return true; }
    inline bool      IsInputEnabled() const { return !HasFlags(FL_INGORE_INPUT); }
    UInputComponent* GetInput();
    void             RegisterInputsToControllers();

    ftime m_tLastTimeUsed;

    // overriden by functions declared in ABaseEntity
    virtual FEntityInputRegistrationParams*
    GetLeftControllerInputRegistrationParams() {
        return NULL;
    }
    virtual FEntityInputRegistrationParams*
    GetRightControllerInputRegistrationParams() {
        return NULL;
    }
};

template <class T> inline T* ExtractArg(void* pParam) {
    return reinterpret_cast<T*>(pParam);
}

// finds a UStaticMesh by path
UStaticMesh* FindMesh(const wchar_t* path);
UStaticMesh* FindMesh(const char* path);
