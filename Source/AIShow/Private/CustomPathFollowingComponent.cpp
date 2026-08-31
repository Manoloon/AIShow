// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomPathFollowingComponent.h"

#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/MetaNavMeshPath.h"

FString GetPathDescHelper(FNavPathSharedPtr Path)
{
	return !Path.IsValid() ? TEXT("missing") : !Path->IsValid() ? TEXT("invalid") : FString::Printf(TEXT("%s:%d"), Path->IsPartial() ? TEXT("partial") : TEXT("complete"), Path->GetPathPoints().Num());
}

float UCustomPathFollowingComponent::DistancePointToSegment2D(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd, FVector& OutClosestPoint)
{
	const FVector2D P(Point.X,Point.Y);
	const FVector2D A(SegmentStart.X,SegmentStart.Y);
	const FVector2D B(SegmentEnd.X,SegmentEnd.Y);
	
	const FVector2D AB = B - A;
	const float ABLengthSquared = AB.SizeSquared();
	if (ABLengthSquared <= KINDA_SMALL_NUMBER)
	{
		OutClosestPoint = SegmentStart;
		return FVector2D::Distance(P,A);
	}
	const float T = FVector2D::DotProduct(P - A,AB) / ABLengthSquared;
	const float TClamped = FMath::Clamp(T,0.0f,1.0f);
	const FVector2D ClosestPoint = A + AB * TClamped;
	OutClosestPoint = FVector{ClosestPoint.X,ClosestPoint.Y,SegmentStart.Z};
	return FVector2D::Distance(P,ClosestPoint);
}

bool UCustomPathFollowingComponent::SegmentIntersectsVisionCone2D(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& PlayerLocation, const FVector& PlayerForward) const
{
	const FVector2D Start(SegmentStart.X,SegmentStart.Y);
	const FVector2D End(SegmentEnd.X,SegmentEnd.Y);
	const FVector2D Player(PlayerLocation.X,PlayerLocation.Y);
	FVector2D Forward(PlayerForward.X,PlayerForward.Y);
	Forward.Normalize();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfVisionCone));
	const FVector2D Segment = End - Start;
	
	auto IsPointInsideCone = [&](const FVector2D& Point) -> bool 
	{
		const FVector2D ToPoint = Point - Player;
		const float DistanceSquared = ToPoint.SizeSquared();
		
		// out of radius
		if (DistanceSquared > FMath::Square(ConeDistance))
		{
			UE_LOG(LogTemp,Error,TEXT("[VISION] out of radius"));
			return false;
		}
		if (DistanceSquared <= KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogTemp,Warning,TEXT("[VISION] DistanceSquared almost 0"));
			return true;
		}
		const FVector2D Direction = ToPoint.GetSafeNormal();
		const float Dot = FVector2D::DotProduct(Forward,Direction);
		return Dot >= CosHalfAngle;
	};
	if (IsPointInsideCone(Start))
	{
		UE_LOG(LogTemp,Warning,TEXT("[VISION] IsPointInsideCone -> Start"));
		return true;
	}
	if (IsPointInsideCone(End))
	{
		UE_LOG(LogTemp,Warning,TEXT("[VISION] IsPointInsideCone -> End"));
		return true;
	}
	// Interseccion segmento / circulo
	const FVector2D StartToPlayer = Start - Player;
	const float A = FVector2D::DotProduct(Segment,Segment);
	if (A <+ KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float B = 2.f * FVector2D::DotProduct(StartToPlayer,Segment);
	const float C = FVector2D::DotProduct(StartToPlayer,StartToPlayer) - FMath::Square(ConeDistance);
	const float Discriminant = B * B - 4.0f * A * C;
	
	if (Discriminant < 0.0f)
	{
		return false;
	}
	
	const float SqrtDiscriminant = FMath::Sqrt(Discriminant);
	const float Inv2A = 1.0f / (2.0f * A);
	const float T1 = (-B - SqrtDiscriminant) * Inv2A;
	const float T2 = (-B + SqrtDiscriminant) * Inv2A;
	
	// Check points where the segment enters and out of circle
	if (T1 >= 0.0f && T1 <= 1.0f)
	{
		const FVector2D Intersection = Start + Segment * T1;
		
		if (IsPointInsideCone(Intersection))
		{
			////////////////////////////////////////////////////////////////// For debug
			const FVector2D ToIntersection = Intersection - Player;
			const float Distance =	ToIntersection.Size();
			const FVector2D Direction =	Intersection.GetSafeNormal();
			const float Dot =FVector2D::DotProduct(Forward,Direction);
			const float Angle =	FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
			UE_LOG(LogTemp,Warning,TEXT("[VISION] T1 = %.4f | Point=(%.2f, %.2f) | Distance=%.2f | Dot=%.4f | Angle=%.2f"),
				T1,Intersection.X,Intersection.Y,Distance,Dot,Angle);
			//////////////////////////////////////////////////////////////////////////////
			return true;
		}
	}
	if (T2 >= 0.0f && T2 <= 1.0f)
	{
		const FVector2D Intersection = Start + Segment * T2;
		if (IsPointInsideCone(Intersection))
		{
			UE_LOG(LogTemp,Warning,TEXT("[VISION] Intersection T2"));
			return true;
		}
	}
	return false;
}

FAIRequestID UCustomPathFollowingComponent::RequestMove(const FAIMoveRequest& RequestData, FNavPathSharedPtr InPath)
{
	CurrentMoveRequest = RequestData;
	UE_LOG(LogTemp,Error,TEXT(">>> Custom RequestMove"));
	if(InPath.IsValid() && IsPlayerCrossing(InPath) && !bIsRepathing)
	{
		UE_LOG(LogTemp,Error,TEXT(">>> IsPlayerCrossing"));
		bIsRepathing = true;
		// abort current move
		AbortMove(*this,FPathFollowingResultFlags::MovementStop,GetCurrentRequestId(),EPathFollowingVelocityMode::Reset);
		GetWorld()->GetTimerManager().SetTimerForNextTick(this,&UCustomPathFollowingComponent::ExecuteRepath);
		return FAIRequestID();
	}
	return Super::RequestMove(RequestData, InPath);
}

void UCustomPathFollowingComponent::ExecuteRepath()
{
	static int32 RepathCounter = 0;
	++RepathCounter;

	UE_LOG(
		LogTemp,
		Error,
		TEXT("========== EXECUTING REPATH #%d =========="),
		RepathCounter);
	UE_LOG(
	LogTemp,
	Error,
	TEXT("[REPATH] TimerManager HasPendingTimer = %s"),
	GetWorld()->GetTimerManager().IsTimerPending(RepathTimerHandle)
		? TEXT("TRUE")
		: TEXT("FALSE"));
	
	
	AAIController* AIContr = Cast<AAIController>(GetOwner());
	
	if (!IsValid(AIContr) || !CurrentMoveRequest.IsValid())
	{
		bIsRepathing = false;
		return;
	}

	UE_LOG(LogTemp,Error,TEXT("========== EXECUTING REPATH =========="));
	const FPathFollowingRequestResult Result = AIContr->MoveTo(CurrentMoveRequest);
	UE_LOG(LogTemp,Error,TEXT(">>> REPATH RESULT CODE: %d"),static_cast<int32>(Result.Code));
	UE_LOG(LogTemp,Error,TEXT(">>> REPATH REQUEST ID: %u"),Result.MoveId.GetID());
	bIsRepathing = false;
}

bool UCustomPathFollowingComponent::IsPlayerCrossing(FNavPathSharedPtr InPath)
{
	// Ensure the player character and path following component are valid
	const ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter || !InPath.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Player OR Path are NOT Valid"));
		return false;
	}
	const TArray<FNavPathPoint>& Points = InPath->GetPathPoints();
	if (Points.Num() < 2)
	{
		return false;
	}
	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	// Convertir a un cono de 45.f grados
	// 		ángulo      Dot
	//		0°          1.0
	//		45°         0.707
	//		90°         0.0
	//		180°       -1.0
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
	
	/// debug cone
	const FVector LeftBoundary = PlayerForward.RotateAngleAxis(-HalfVisionCone, FVector::UpVector);
	const FVector RightBoundary = PlayerForward.RotateAngleAxis(HalfVisionCone, FVector::UpVector);
	DrawDebugSphere(GetWorld(),PlayerLocation,60,12,FColor::Blue,true,1.0f,1,2);
	DrawDebugLine(
		GetWorld(),
		PlayerLocation,
		PlayerLocation + (PlayerForward * ConeDistance),
		FColor::Blue,
		true,
		1.0f,
		1,
		4.0f);
	DrawDebugSphere(GetWorld(),PlayerLocation,60,12,FColor::Green,true,1.0f,1,2);
	DrawDebugLine(
		GetWorld(),
		PlayerLocation,
		PlayerLocation + (LeftBoundary * ConeDistance),
		FColor::Green,
		true,
		1.0f,
		1.f,
		3.0f);
	DrawDebugSphere(GetWorld(),PlayerLocation,60,12,FColor::Green,true,1.0f,1,2);
	DrawDebugLine(
		GetWorld(),
		PlayerLocation,
		PlayerLocation + (RightBoundary * ConeDistance),
		FColor::Green,
		true,
		1.0f,
		1.f,
		3.0f);
	///// debug
	for (int32 i = 0; i < Points.Num() - 1;++i)
	{
	////////////////////////////////////////////////////////////// Debug
			DrawDebugLine(
				GetWorld(),
				Points[i].Location + FVector(0, 0, 10),
				Points[i + 1].Location + FVector(0, 0, 10),
				FColor::White,
				true,
				5.0f,
				SDPG_Foreground,
				8.0f);
		/////////////////////////////////////////////////////////////////////////////
		///
		const FVector Start = Points[i].Location;
		const FVector End = Points[i + 1].Location;
		if (SegmentIntersectsVisionCone2D(Start,End,PlayerLocation,PlayerForward))
		{
			UE_LOG(LogTemp,Warning,TEXT("PATH INTERSECTS PLAYER VISION CONE"));
			return true;
		}
	}
	return false;
}
