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

protected:
	static float DistancePointToSegment2D(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd, FVector& OutClosestPoint);
	bool SegmentIntersectsVisionCone2D(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& PlayerLocation, const FVector& PlayerForward) const;
	virtual bool HandlePathUpdateEvent() override;
	FTimerHandle RepathTimerHandle;
	
private:
	bool CreateAvoidanceMetaPath(const FNavPathSharedPtr& OriginalPath, FNavPathSharedPtr& OutMetaPath);
	bool GetAvoidanceWaypoint(const FVector& Start, const FVector& PlayerLocation, const FVector& PlayerForward, 
								const FVector& Goal,FVector& OutAvoidPoint,FVector& OutAvoidPoint2);
};
