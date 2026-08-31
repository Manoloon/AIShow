// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_AdjacentsPoints.generated.h"

/**
 * check if node is adjacent.
 */
UCLASS()
class AISHOW_API UEnvQueryTest_AdjacentsPoints : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Distance")
	float DistanceThreshold = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Distance")
	bool scored = false;
	
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	virtual void PostLoad() override;
};
