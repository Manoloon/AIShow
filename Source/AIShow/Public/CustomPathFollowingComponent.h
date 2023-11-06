// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "CustomPathFollowingComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AISHOW_API UCustomPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

	FAIRequestID CurrentRequestId;
	/** timer handle for OnWaitingPathTimeout function */
	FTimerHandle WaitingForPathTimer;
	float RadiusOfAvoidance = 300.f;
	
public:
	// Sets default values for this component's properties
	UCustomPathFollowingComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	bool IsPlayerCrossingPath(TArray<FVector>& CurrentLocations);
	bool IsPlayerCrossing(FNavPathSharedPtr& InPath);
	

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual FAIRequestID RequestMove(const FAIMoveRequest& RequestData, FNavPathSharedPtr InPath) override;
	virtual void UpdateMoveFocus() override;
	virtual bool HandlePathUpdateEvent();
};
