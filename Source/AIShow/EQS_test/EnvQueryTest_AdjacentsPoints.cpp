// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryTest_AdjacentsPoints.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"

UEnvQueryTest_AdjacentsPoints::UEnvQueryTest_AdjacentsPoints(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	//SetWorkOnFloatValues(false);
}

void UEnvQueryTest_AdjacentsPoints::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (QueryOwner == nullptr)
	{
		return;
	}

	BoolValue.BindData(QueryInstance.Owner.Get(), QueryInstance.QueryID);
	const bool bWantsAdjacent = BoolValue.GetValue();

	//TArray<FVector> Points;

	// for(FEnvQueryInstance::ItemIterator It(this,QueryInstance);It;++It)
	// {
	// 	FVector ItemLocation = GetItemLocation(QueryInstance,It.GetIndex());
	// 	Points.Add(ItemLocation);
	// }
	FVector lastPoint = FVector::ZeroVector;
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());

		//Points.Add(ItemLocation);
		bool bIsAdjacent = false;
		float Score = 0.0f;

		if (lastPoint == FVector::ZeroVector)
		{
			lastPoint = ItemLocation;
			continue;
		}
		if (FVector::Dist(ItemLocation, lastPoint) <= DistanceThreshold)
		{
			bIsAdjacent = true;
			Score += 1.0f;
			break;
		}
		if (scored)
		{
			It.SetScore(TestPurpose, FilterType, Score, 0.0f, QueryInstance.NumProcessedItems);
		}
		else
		{
			// expected vs the reality.
			It.SetScore(TestPurpose, FilterType, bIsAdjacent, bWantsAdjacent);
		}
	}
}

FText UEnvQueryTest_AdjacentsPoints::GetDescriptionTitle() const
{
	//const FString scoring = scored?"true":"false";
	return FText::FromString(FString::Printf(TEXT("%s: %f"),
		*Super::GetDescriptionTitle().ToString(), DistanceThreshold));
}

FText UEnvQueryTest_AdjacentsPoints::GetDescriptionDetails() const
{
	return DescribeBoolTestParams(BoolValue.ValueToString());
}

void UEnvQueryTest_AdjacentsPoints::PostLoad()
{
	Super::PostLoad();
}
