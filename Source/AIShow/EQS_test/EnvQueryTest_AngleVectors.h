// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_AngleVectors.generated.h"

/**
*	check: smallest(angle(direction from querier to target,direction from querier to point))
*	angle from 2 Vectors = acosd(dotproduct(vectorA.normalize(),vectorB.normalize)))
 */
UCLASS()
class AISHOW_API UEnvQueryTest_AngleVectors : public UEnvQueryTest
{
	GENERATED_BODY()
	//
	// /** defines direction of first line used by test */
	// UPROPERTY(EditDefaultsOnly, Category=Angle)
	// FEnvDirection LineA;
	//
	// /** defines direction of second line used by test */
	// UPROPERTY(EditDefaultsOnly, Category=Angle)
	// FEnvDirection LineB;
	//
	// AIMODULE_API virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	//
	// AIMODULE_API virtual FText GetDescriptionTitle() const override;
	// AIMODULE_API virtual FText GetDescriptionDetails() const override;
	//
	// AIMODULE_API virtual void PostLoad() override;
	//
	// /** helper function: gather directions from context pairs */
	// void GatherLineDirections(TArray<FVector>& Directions, FEnvQueryInstance& QueryInstance, const FVector& ItemLocation,
	// 	TSubclassOf<UEnvQueryContext> LineFrom, TSubclassOf<UEnvQueryContext> LineTo) const;
	//
	// /** helper function: gather directions from context */
	// void GatherLineDirections(TArray<FVector>& Directions, FEnvQueryInstance& QueryInstance, const FRotator& ItemRotation,
	// 	TSubclassOf<UEnvQueryContext> LineDirection) const;
	//
	// /** helper function: gather directions from proper contexts */
	// void GatherLineDirections(TArray<FVector>& Directions, FEnvQueryInstance& QueryInstance,
	// 	TSubclassOf<UEnvQueryContext> LineFrom, TSubclassOf<UEnvQueryContext> LineTo, TSubclassOf<UEnvQueryContext> LineDirection, bool bUseDirectionContext,
	// 	const FVector& ItemLocation = FVector::ZeroVector, const FRotator& ItemRotation = FRotator::ZeroRotator) const;
	//
	// /** helper function: check if contexts are updated per item */
	// bool RequiresPerItemUpdates(TSubclassOf<UEnvQueryContext> LineFrom, TSubclassOf<UEnvQueryContext> LineTo, TSubclassOf<UEnvQueryContext> LineDirection, bool bUseDirectionContext) const;
	//
};
