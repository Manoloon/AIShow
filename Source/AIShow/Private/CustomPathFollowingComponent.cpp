// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomPathFollowingComponent.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/MetaNavMeshPath.h"
#include "VisualLogger/VisualLogger.h"


// Sets default values for this component's properties
UCustomPathFollowingComponent::UCustomPathFollowingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCustomPathFollowingComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}


// Called every frame
void UCustomPathFollowingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FString GetPathDescHelper(FNavPathSharedPtr Path)
{
	return !Path.IsValid() ? TEXT("missing") : !Path->IsValid() ? TEXT("invalid") : FString::Printf(TEXT("%s:%d"), Path->IsPartial() ? TEXT("partial") : TEXT("complete"), Path->GetPathPoints().Num());
}

FAIRequestID UCustomPathFollowingComponent::RequestMove(const FAIMoveRequest& RequestData, FNavPathSharedPtr InPath)
{
//	InPath->GetGoalLocation();
	InPath->GetPathPointLocation()
	
	TArray<FVector> Locations;
	AController* Controller = Cast<AController>(GetOwner());
	ACharacter* Character = Controller->GetCharacter();
	if(Character)
	{
		FVector CharLoc = Character->GetActorLocation();
		Locations.Add(FVector(CharLoc.X+50.f, CharLoc.Y-50.f, 0.f));
		Locations.Add(FVector(CharLoc.X+250.f, CharLoc.Y-250.f, 0.f));
		Locations.Add(FVector(CharLoc.X, CharLoc.Y, 0.f));
	}
	//Locations.Add(FVector(50.f, 50.f, 0.f));
	//Locations.Add(FVector(450.f, -250.f, 0.f));
	//Locations.Add(FVector(50.f, 50.f, 0.f));
	//Locations.Add(FVector(650.f, 250.f, 0.f));

	
	if (IsPlayerCrossingPath(Locations))
	{
		UE_LOG(LogTemp, Warning, TEXT("Crossing!"));
	}
	FMetaNavMeshPath* MetaNavMeshPath = new FMetaNavMeshPath(Locations, *Controller);
	TSharedPtr<FMetaNavMeshPath, ESPMode::ThreadSafe> MetaPathPtr(MetaNavMeshPath);

	InPath = FNavPathSharedPtr(MetaPathPtr);
	return Super::RequestMove(RequestData, MetaPathPtr);
}

void UCustomPathFollowingComponent::UpdateMoveFocus()
{
	Super::UpdateMoveFocus();
}

bool UCustomPathFollowingComponent::HandlePathUpdateEvent()
{
	return Super::HandlePathUpdateEvent();
}


bool UCustomPathFollowingComponent::IsPlayerCrossingPath(TArray<FVector>& CurrentLocations)
{
	// Ensure the player character and path following component are valid
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is NOT Valid"));
		return false;
	}

	// Get the player character's location
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	for(FVector& CurrentLocation : CurrentLocations)
	{
		float Dist = FVector::Dist(PlayerLocation, CurrentLocation);
		DrawDebugSphere(GetWorld(), CurrentLocation, 10, 6, FColor::Green, true, 1, 0, 3);
		if (Dist < CrossThresholdDistance)
		{
			UE_LOG(LogTemp, Error, TEXT("Player is in the path , %f"), Dist);
			CurrentLocation.X += 200.0f;
			DrawDebugSphere(GetWorld(), CurrentLocation, 10, 6, FColor::Magenta, true, 1, 0, 3);
			return true;
		}
		UE_LOG(LogTemp, Warning, TEXT("Player is not in the Way , %f"), Dist);
		return false;
	}
	return false;
}

bool UCustomPathFollowingComponent::IsPlayerCrossing(FNavPathSharedPtr InPath)
{
	// Ensure the player character and path following component are valid
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is NOT Valid"));
		return false;
	}

	// Get the player character's location
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	for(int i= 0; i < InPath->GetLength();i++)
	{
		float Dist = FVector::Dist(PlayerLocation, InPath->GetPathPointLocation(i).CachedBaseLocation);
		DrawDebugSphere(GetWorld(), InPath->GetPathPointLocation(i).CachedBaseLocation, 10, 6, FColor::Green, true, 1, 0, 3);
		if (Dist < CrossThresholdDistance)
		{
			UE_LOG(LogTemp, Error, TEXT("Player is in the path , %f"), Dist);
			//CurrentLocation.X += 200.0f;
			//DrawDebugSphere(GetWorld(), CurrentLocation, 10, 6, FColor::Magenta, true, 1, 0, 3);
			return true;
		}
		UE_LOG(LogTemp, Warning, TEXT("Player is not in the Way , %f"), Dist);
		return false;
	}
	return false;
}
