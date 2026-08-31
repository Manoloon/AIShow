// Fill out your copyright notice in the Description page of Project Settings.
#include "CustomPathFollowingComponent.h"
#include "Navigation/MetaNavMeshPath.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

FString GetPathDescHelper(FNavPathSharedPtr Path)
{
	return !Path.IsValid() ? TEXT("missing") : !Path->IsValid() ? TEXT("invalid") : FString::Printf(TEXT("%s:%d"), Path->IsPartial() ? TEXT("partial") : TEXT("complete"), Path->GetPathPoints().Num());
}

float UCustomPathFollowingComponent::DistancePointToSegment2D(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd, FVector& OutClosestPoint)
{
	const FVector2D P(Point.X, Point.Y);
	const FVector2D A(SegmentStart.X, SegmentStart.Y);
	const FVector2D B(SegmentEnd.X, SegmentEnd.Y);

	const FVector2D AB = B - A;
	const float ABLengthSquared = AB.SizeSquared();
	if (ABLengthSquared <= KINDA_SMALL_NUMBER)
	{
		OutClosestPoint = SegmentStart;
		return FVector2D::Distance(P, A);
	}
	const float T = FVector2D::DotProduct(P - A, AB) / ABLengthSquared;
	const float TClamped = FMath::Clamp(T, 0.0f, 1.0f);
	const FVector2D ClosestPoint = A + AB * TClamped;
	OutClosestPoint = FVector{ ClosestPoint.X, ClosestPoint.Y, SegmentStart.Z };
	return FVector2D::Distance(P, ClosestPoint);
}

bool UCustomPathFollowingComponent::SegmentIntersectsVisionCone2D(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& PlayerLocation, const FVector& PlayerForward) const
{
	const FVector2D Start(SegmentStart.X, SegmentStart.Y);
	const FVector2D End(SegmentEnd.X, SegmentEnd.Y);
	const FVector2D Player(PlayerLocation.X, PlayerLocation.Y);
	FVector2D Forward(PlayerForward.X, PlayerForward.Y);
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
			UE_LOG(LogTemp, Error, TEXT("[VISION] out of radius"));
			return false;
		}
		if (DistanceSquared <= KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogTemp, Warning, TEXT("[VISION] DistanceSquared almost 0"));
			return true;
		}
		const FVector2D Direction = ToPoint.GetSafeNormal();
		const float Dot = FVector2D::DotProduct(Forward, Direction);
		return Dot >= CosHalfAngle;
	};
	if (IsPointInsideCone(Start))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VISION] IsPointInsideCone -> Start"));
		return true;
	}
	if (IsPointInsideCone(End))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VISION] IsPointInsideCone -> End"));
		return true;
	}
	// Interseccion segmento / circulo
	const FVector2D StartToPlayer = Start - Player;
	const float A = FVector2D::DotProduct(Segment, Segment);
	if (A < +KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float B = 2.f * FVector2D::DotProduct(StartToPlayer, Segment);
	const float C = FVector2D::DotProduct(StartToPlayer, StartToPlayer) - FMath::Square(ConeDistance);
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
			const float Distance = ToIntersection.Size();
			const FVector2D Direction = ToIntersection.GetSafeNormal();
			const float Dot = FVector2D::DotProduct(Forward, Direction);
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
			UE_LOG(LogTemp, Warning, TEXT("[VISION] T1 = %.4f | Point=(%.2f, %.2f) | Distance=%.2f | Dot=%.4f | Angle=%.2f"),
				T1, Intersection.X, Intersection.Y, Distance, Dot, Angle);
			//////////////////////////////////////////////////////////////////////////////
			return true;
		}
	}
	if (T2 >= 0.0f && T2 <= 1.0f)
	{
		const FVector2D Intersection = Start + Segment * T2;
		if (IsPointInsideCone(Intersection))
		{
			UE_LOG(LogTemp, Warning, TEXT("[VISION] Intersection T2"));
			return true;
		}
	}
	return false;
}

bool UCustomPathFollowingComponent::HandlePathUpdateEvent()
{
	UE_LOG(LogTemp, Error, TEXT(">>> HandlePathUpdateEvent"));
	if (Path.IsValid())
	{
		const TArray<FNavPathPoint>& Points = Path->GetPathPoints();
		UE_LOG(LogTemp, Error, TEXT("[PATH] | Points: %d | Partial: %s"), Path->GetPathPoints().Num(), Path->IsPartial() ? TEXT("TRUE") : TEXT("FALSE"));

		for (int32 i = 0; i < Points.Num(); ++i)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PATH] Point[%d] = %s"), i, *Points[i].Location.ToString());
			if (i < Points.Num() - 1)
			{
				DrawDebugLine(GetWorld(), Points[i].Location + FVector(0, 0, 20), Points[i + 1].Location + FVector(0, 0, 20), FColor::Yellow, true, 5.0f, SDPG_Foreground, 10.0f);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PATH] Path INVALID"));
	}
	return Super::HandlePathUpdateEvent();
}

void UCustomPathFollowingComponent::StartAvoidanceUpdates()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(AvoidanceUpdateTH)) return;
	GetWorld()->GetTimerManager().SetTimer(AvoidanceUpdateTH,this,&UCustomPathFollowingComponent::UpdateAvoidancePath,0.15f,true);
}

void UCustomPathFollowingComponent::StopAvoidanceUpdates()
{
	GetWorld()->GetTimerManager().ClearTimer(AvoidanceUpdateTH);
	bUsingAvoidancePath = false;
}

void UCustomPathFollowingComponent::UpdateAvoidancePath()
{
	if (!bUsingAvoidancePath) return;
	if (!AIController || !PlayerCharacter)
	{
		StopAvoidanceUpdates();
		return;
	}
	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	const float PlayerMoveDistance = FVector::Dist2D(PlayerLocation,LastAvoidancePlayerlocation);
	if (PlayerMoveDistance < DistanceToPlayerThreshold) return;
	
	UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] Player moved %.2f units -> REBUILD PATH"), PlayerMoveDistance);
	LastAvoidancePlayerlocation = PlayerLocation;
	const FVector EnemyLocation = AIController->GetPawn()->GetActorLocation();
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
	
	FVector AvoidPoint1;
	FVector AvoidPoint2;
	
	if (!GetAvoidanceWaypoint(EnemyLocation,PlayerLocation,PlayerForward,CurrentPathGoal,AvoidPoint1,AvoidPoint2))
	{
		return;
	}
	
	TArray<FVector> Waypoints;
	Waypoints.Reserve(4);
	Waypoints.Add(EnemyLocation);
	Waypoints.Add(AvoidPoint1);
	Waypoints.Add(AvoidPoint2);
	Waypoints.Add(CurrentMoveRequest.GetGoalLocation());
	
	TSharedPtr<FMetaNavMeshPath,ESPMode::ThreadSafe> MetaPath = MakeShared<FMetaNavMeshPath>(Waypoints,*AIController);
	if (!MetaPath.IsValid()) return;
	
	UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] NEW META PATH: Enemy=%s Avoid1=%s Avoid2=%s Goal=%s"), *EnemyLocation.ToString(), *AvoidPoint1.ToString(), *AvoidPoint2.ToString(), *CurrentMoveRequest.GetGoalLocation().ToString());

	//RequestMove(CurrentMoveRequest,MetaPath);
}

bool UCustomPathFollowingComponent::CreateAvoidanceMetaPath(const FNavPathSharedPtr& OriginalPath, FNavPathSharedPtr& OutMetaPath)
{
	if (!OriginalPath.IsValid())
		return false;

	AController* Controller = GetOwner<AController>();
	if (!Controller)
		return false;

	const TArray<FNavPathPoint>& Points = OriginalPath->GetPathPoints();
	if (Points.Num() < 2)
		return false;

	const FVector Start = Points[0].Location;
	const FVector Goal = Points.Last().Location;
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter)
		return false;

	LastAvoidancePlayerlocation = PlayerCharacter->GetActorLocation();
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();

	FVector AvoidPoint;
	FVector AvoidPoint2;
	if (!GetAvoidanceWaypoint(Start, LastAvoidancePlayerlocation, PlayerForward,
								Goal,AvoidPoint,AvoidPoint2))
	{
		return false;
	}
	
	TArray<FVector> Waypoints;
	Waypoints.Reserve(4);
	Waypoints.Add(Start);
	Waypoints.Add(AvoidPoint);
	Waypoints.Add(AvoidPoint2);
	Waypoints.Add(CurrentPathGoal);

	TSharedPtr<FMetaNavMeshPath, ESPMode::ThreadSafe> MetaPath = MakeShared<FMetaNavMeshPath>(Waypoints, *Controller);
	if (!MetaPath.IsValid())
		return false;

	OutMetaPath = MetaPath;

	UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] META PATH CREATED: Start=%s Avoid=%s Goal=%s"), *Start.ToString(), *AvoidPoint.ToString(), *Goal.ToString());
	const FVector Offset = FVector(0, 0, 20);
	DrawDebugLine(GetWorld(), Start + Offset, AvoidPoint + Offset, FColor::Yellow, false, 5.0f, SDPG_Foreground, 10.0f);
	DrawDebugLine(GetWorld(), AvoidPoint + Offset, Goal + Offset, FColor::Yellow, false, 5.0f, SDPG_Foreground, 10.0f);
	DrawDebugLine(GetWorld(), AvoidPoint2 + Offset, Goal + Offset, FColor::Yellow, false, 5.0f, SDPG_Foreground, 10.0f);
	return true;
}

bool UCustomPathFollowingComponent::GetAvoidanceWaypoint(const FVector& Start, const FVector& PlayerLocation, const FVector& PlayerForward, 
																		const FVector& Goal,FVector& OutAvoidPoint,FVector& OutAvoidPoint2)
{
	 const FVector Forward = PlayerForward.GetSafeNormal2D();
    if (Forward.IsNearlyZero()) return false;

    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal2D();

    const FVector ToEnemy = Start - PlayerLocation;

    const float ForwardDistance = FVector::DotProduct(ToEnemy, Forward);
    const float SideDistance = FVector::DotProduct(ToEnemy, Right);

    if (ForwardDistance <= 0.0f) return false;

    const float HalfAngleRadians = FMath::DegreesToRadians(HalfVisionCone);
    const float ConeHalfWidth = ForwardDistance * FMath::Tan(HalfAngleRadians);

    const float SideSign = SideDistance >= 0.0f ? 1.0f : -1.0f;
    const float Margin = 150.0f;

    const float EscapeSide = ConeHalfWidth + Margin;

    OutAvoidPoint = PlayerLocation + Forward * ForwardDistance + Right * (EscapeSide * SideSign);
    OutAvoidPoint2 = PlayerLocation + Forward * (ForwardDistance - 150.0f) + Right * (EscapeSide * SideSign);

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return false;

    FNavLocation Projected1;
    FNavLocation Projected2;

    if (!NavSys->ProjectPointToNavigation(OutAvoidPoint, Projected1)) return false;
    if (!NavSys->ProjectPointToNavigation(OutAvoidPoint2, Projected2)) return false;

    OutAvoidPoint = Projected1.Location;
    OutAvoidPoint2 = Projected2.Location;

    DrawDebugSphere(GetWorld(), OutAvoidPoint, 35.0f, 16, FColor::Yellow, false, 5.0f, 1, 4.0f);
    DrawDebugSphere(GetWorld(), OutAvoidPoint2, 35.0f, 16, FColor::Yellow, false, 5.0f, 1, 4.0f);
	const FVector Offset = FVector(0, 0, 20);
    DrawDebugLine(GetWorld(), Start + Offset, OutAvoidPoint + Offset, FColor::Yellow, false, 5.0f, 1, 6.0f);
    DrawDebugLine(GetWorld(), OutAvoidPoint + Offset, OutAvoidPoint2 + Offset, FColor::Yellow, false, 5.0f, 1, 6.0f);

    UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] ForwardDistance=%.2f SideDistance=%.2f ConeHalfWidth=%.2f"), ForwardDistance, SideDistance, ConeHalfWidth);
    UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] AvoidPoint1 = %s"), *OutAvoidPoint.ToString());
    UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] AvoidPoint2 = %s"), *OutAvoidPoint2.ToString());

    return true;
}

FAIRequestID UCustomPathFollowingComponent::RequestMove(const FAIMoveRequest& RequestData, FNavPathSharedPtr InPath)
{
	CurrentMoveRequest = RequestData;
	if (InPath.IsValid() && InPath->GetPathPoints().Num() > 0)
	{
		CurrentPathGoal = InPath->GetPathPoints().Last().Location;
		UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] Original Goal = %s"), *CurrentPathGoal.ToString());
	}
	UE_LOG(LogTemp, Error, TEXT(">>> Custom RequestMove"));
	if (InPath.IsValid() && IsPlayerCrossing(InPath) && !bIsRepathing)
	{
		UE_LOG(LogTemp, Error, TEXT(">>> IsPlayerCrossing"));
		FNavPathSharedPtr AvoidancePath;
		if (CreateAvoidanceMetaPath(InPath, AvoidancePath))
		{
			LastAvoidancePlayerlocation = UGameplayStatics::GetPlayerCharacter(GetWorld(),0)->GetActorLocation();
			bUsingAvoidancePath = true;
			StartAvoidanceUpdates();
			UE_LOG(LogTemp, Error, TEXT(">>> Avoidance MetaPath successfully created"));
			return Super::RequestMove(RequestData, AvoidancePath);
		}
	}
	return Super::RequestMove(RequestData, InPath);
}

void UCustomPathFollowingComponent::ExecuteRepath()
{
	static int32 RepathCounter = 0;
	++RepathCounter;

	UE_LOG(LogTemp,Error,TEXT("========== EXECUTING REPATH #%d =========="),RepathCounter);
	AAIController* AIContr = Cast<AAIController>(GetOwner());
	if (!IsValid(AIContr) || !CurrentMoveRequest.IsValid())
	{
		bIsRepathing = false;
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("========== EXECUTING REPATH =========="));
	const FPathFollowingRequestResult Result = AIContr->MoveTo(CurrentMoveRequest);
	UE_LOG(LogTemp, Error, TEXT(">>> REPATH RESULT CODE: %d"), static_cast<int32>(Result.Code));
	UE_LOG(LogTemp, Error, TEXT(">>> REPATH REQUEST ID: %u"), Result.MoveId.GetID());
	bIsRepathing = false;
}

bool UCustomPathFollowingComponent::IsPlayerCrossing(FNavPathSharedPtr InPath)
{
	AIController = Cast<AAIController>(GetOwner());
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(),0);
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
	LastAvoidancePlayerlocation = PlayerCharacter->GetActorLocation();
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
	const FVector EndLeftWhisker = LastAvoidancePlayerlocation + (LeftBoundary * ConeDistance);
	const FVector EndRightWhisker = LastAvoidancePlayerlocation + (RightBoundary * ConeDistance);
	const FVector EndPlayerVision = LastAvoidancePlayerlocation + (PlayerForward * ConeDistance);
	DrawDebugSphere(GetWorld(), LastAvoidancePlayerlocation, 60, 12, FColor::Blue, true, 1.0f, 1, 2);
	DrawDebugLine(GetWorld(),LastAvoidancePlayerlocation,EndPlayerVision,FColor::Blue,true,1.0f,1,3.0f);
	DrawDebugSphere(GetWorld(), LastAvoidancePlayerlocation, 60, 12, FColor::Green, true, 1.0f, 1, 2);
	DrawDebugLine(GetWorld(),LastAvoidancePlayerlocation,EndLeftWhisker,FColor::Green,true,1.0f,1.f,2.0f);
	DrawDebugSphere(GetWorld(), LastAvoidancePlayerlocation, 60, 12, FColor::Green, true, 1.0f, 1, 2);
	DrawDebugLine(GetWorld(),LastAvoidancePlayerlocation,EndRightWhisker,FColor::Green,true,1.0f,1.f,2.0f);
	DrawDebugLine(GetWorld(),EndLeftWhisker,EndRightWhisker,FColor::Green,true,1.0f,1.f,2.0f);
	
	///// debug
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		////////////////////////////////////////////////////////////// Debug
		const FVector LocOffset = FVector(0, 0, 10);
		DrawDebugLine(GetWorld(),Points[i].Location + LocOffset,Points[i + 1].Location + LocOffset,FColor::White,true,5.0f,SDPG_Foreground,8.0f);
		////////////////////////////////////////////////////////////////////////////
		const FVector Start = Points[i].Location;
		const FVector End = Points[i + 1].Location;
		// TODO : creo que deberiamos ver si cruza con el total del cono y no con el playerLocation
		if (SegmentIntersectsVisionCone2D(Start, End, LastAvoidancePlayerlocation, PlayerForward))
		{
			UE_LOG(LogTemp, Warning, TEXT("PATH INTERSECTS PLAYER VISION CONE"));
			return true;
		}
	}
	return false;
}
