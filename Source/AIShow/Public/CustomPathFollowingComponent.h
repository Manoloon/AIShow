// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "CustomPathFollowingComponent.generated.h"

class AAIController;

UCLASS(ClassGroup=(Navigation), meta=(BlueprintSpawnableComponent))
class AISHOW_API UCustomPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()
public:
	virtual FAIRequestID RequestMove(const FAIMoveRequest& RequestData, FNavPathSharedPtr InPath) override;
	virtual void OnPathFinished(const FPathFollowingResult& Result) override;

private:
	bool IsPlayerCrossing(FNavPathSharedPtr InPath);
	bool SegmentIntersectsVisionCone2D(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& PlayerLocation, const FVector& PlayerForward);
	bool CreateAvoidanceMetaPath(const FNavPathSharedPtr& OriginalPath, FNavPathSharedPtr& OutMetaPath);
	bool GetAvoidanceWaypoint(const FVector& Start, const FVector& PlayerLocation, const FVector& PlayerForward,
		FVector& OutAvoidPoint);
	void StartAvoidanceUpdates();
	void UpdateAvoidancePath();
	void StopAvoidanceUpdates();

	#if !UE_BUILD_SHIPPING
	// Debug functions
	void DrawConeOfVision(const FVector& PlayerForwardVector, const FVector& PlayerLocation, bool bPersistentLines = false, float LifeTime = 0.01f) const;
	#endif
	
protected:
	virtual void FollowPathSegment(float DeltaTime) override;
	virtual bool HandlePathUpdateEvent() override;
private:
	UPROPERTY(EditAnywhere,Category = "Settings")
	float HalfVisionCone = 45.0f;
	UPROPERTY(EditAnywhere,Category = "Settings")
	float ConeDistance = 800.f;
	// Its the minimum distance that the player must move to repath
	UPROPERTY(EditAnywhere,Category = "Settings")
	float MinPlayerMovementThreshold = 100.f;
	// Time rate for updating Avoidance Path
	UPROPERTY(EditAnywhere,Category = "Settings")
	float UpdateAvoidancePathRate = 0.15f;
	UPROPERTY(EditAnywhere,Category = "Settings")
	float Margin = 150.f;
	FNavPathSharedPtr CurrentPath;
	FAIMoveRequest CurrentMoveRequest;
	FTimerHandle AvoidanceUpdateTH;
	FVector LastAvoidancePlayerlocation = FVector::ZeroVector;
	bool bUsingAvoidancePath = false;
	UPROPERTY()
	AAIController* AIController = nullptr;
	UPROPERTY()
	ACharacter* PlayerCharacter = nullptr;
	FVector CurrentPathGoal = FVector::ZeroVector;
	FVector2D CurrentIntersection = FVector2D::ZeroVector;
};
