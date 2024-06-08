// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DFNPlayerController.generated.h"


class ADFNCharacter;

UCLASS()
class DEFEATNOMORE_API ADFNPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void UpdatePlayerHealth(float Health, float MaxHealth);
	void UpdatePlayerScore(float Score);
	void UpdatePlayerDefeats(int32 Defeats);
	void UpdateHUDWeaponAmmo(int32 Ammo);
	void UpdateHUDCarriedAmmo(int32 CarriedAmmo);
	void UpdateHUDWeaponImage(UTexture2D* WeaponImage);
	void SetHUDMatchCountDown(float MatchCountDownTime);
	void SetHUDWarmupCountDown(float WarmupCountDownTime);
	void SetHUDCameraMode(FText CameraModeText);

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual float GetServerTime(); //synced with Server World clock

	virtual void ReceivedPlayer() override; // sync with Server Clock as soon as possible.

	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleCooldown();
	
protected:
	virtual void BeginPlay() override;

	void PollInit();
	
	UPROPERTY()
	ADFNCharacter* DFNCharacter;
	
	void SetHUDTime();

	// Sync time between client and Server
	/*
	 * Requests the current server time, passing in the client`s time when the request was sent
	 * @param TimeofClientRequest 
	 */
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeofClientRequest);

	
	/**
	 * Reports the current Server time to the Client and the time when the server received the Client Request
	 * @param TimeofCLientRequest 
	 * @param TimeServerReceivedClientRequest 
	 */
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeofCLientRequest, float TimeServerReceivedClientRequest);

	// Difference between client and server time.
	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaSeconds);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float TimeWarmup, float TimeMatch, float TimeCoolDown, float StartingTime);

private:
	UPROPERTY()
	class ADFNHUD* GameHUD;

	UPROPERTY()
	class ADFNGameMode* DFNGameMode;

	float LevelStartTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	
	uint32 CountDownInt;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UPlayerOverlayWidget* PlayerOverlay;

	bool bInitializedCharacterOverlay = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
};