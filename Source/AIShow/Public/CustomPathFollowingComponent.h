// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "CustomPathFollowingComponent.generated.h"


class AAIController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AISHOW_API UCustomPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

	FAIRequestID CurrentRequestId;
	/** timer handle for OnWaitingPathTimeout function */
	FTimerHandle WaitingForPathTimer;
	float RadiusOfAvoidance = 300.f;
	
	FAIMoveRequest CurrentMoveRequest;
	bool bIsRepathing = false;
	bool bRepathRequested = false;
	void ExecuteRepath();
protected:
	bool IsPlayerCrossing(FNavPathSharedPtr InPath);
public:
	UPROPERTY(EditAnywhere,Category = "Settings")
	float HalfVisionCone = 45.0f;
	UPROPERTY(EditAnywhere,Category = "Settings")
	float ConeDistance = 500.f;
	virtual FAIRequestID RequestMove(const FAIMoveRequest& RequestData, FNavPathSharedPtr InPath) override;
	UPROPERTY(EditAnywhere,Category = "Settings")
	float DistanceToPlayerThreshold = 100.f;
protected:
	static float DistancePointToSegment2D(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd, FVector& OutClosestPoint);
	bool SegmentIntersectsVisionCone2D(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& PlayerLocation, const FVector& PlayerForward) const;
	virtual bool HandlePathUpdateEvent() override;
	FTimerHandle AvoidanceUpdateTH;
	FVector LastAvoidancePlayerlocation = FVector::ZeroVector;
	bool bUsingAvoidancePath = false;
	UPROPERTY()
	AAIController* AIController = nullptr;
	UPROPERTY()
	ACharacter* PlayerCharacter = nullptr;
	FVector CurrentPathGoal = FVector::ZeroVector;
	void StartAvoidanceUpdates();
	void StopAvoidanceUpdates();
	void UpdateAvoidancePath();
	
private:
	bool CreateAvoidanceMetaPath(const FNavPathSharedPtr& OriginalPath, FNavPathSharedPtr& OutMetaPath);
	bool GetAvoidanceWaypoint(const FVector& Start, const FVector& PlayerLocation, const FVector& PlayerForward, 
								const FVector& Goal,FVector& OutAvoidPoint,FVector& OutAvoidPoint2);
};
