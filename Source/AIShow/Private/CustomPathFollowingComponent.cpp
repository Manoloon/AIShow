// Fill out your copyright notice in the Description page of Project Settings.
#include "CustomPathFollowingComponent.h"
#include "Navigation/MetaNavMeshPath.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
namespace AvoidVisionCvars
{
	static bool AvoidVisionDebug = false;
	static FAutoConsoleVariableRef CVarAvoidVisionDebug(
		TEXT("AvoidVision.Debug.Enable"),
		AvoidVisionDebug,
		TEXT("Enable Avoid Vision Cone Debug"));
}
#endif
FAIRequestID UCustomPathFollowingComponent::RequestMove(const FAIMoveRequest& RequestData, FNavPathSharedPtr InPath)
{
	UE_LOG(LogTemp, Error, TEXT(">>> Request Move"));
		
	if (!InPath.IsValid())
	{
		UE_LOG(LogTemp, Warning,TEXT("::RequestMove Path NOT valid -> PASS THROUGH"));
		return Super::RequestMove(RequestData, InPath);
	}
	if (InPath->GetPathPoints().Num() > 0)
	{
		CurrentPathGoal = InPath->GetPathPoints().Last().Location;
		UE_LOG(LogTemp, Warning, TEXT("::RequestMove Original Goal = %s"), *CurrentPathGoal.ToString());
	}
	CurrentMoveRequest = RequestData;
	CurrentPath = InPath;
	if (bUsingAvoidancePath)
	{
		UE_LOG(LogTemp, Warning,TEXT("::RequestMove received while using avoidance path -> PASS THROUGH"));
		return Super::RequestMove(RequestData, InPath);
	}
	if (IsPlayerCrossing(InPath))
	{
		FNavPathSharedPtr AvoidancePath;
		if (CreateAvoidanceMetaPath(InPath, AvoidancePath))
		{
			LastAvoidancePlayerlocation = PlayerCharacter->GetActorLocation();
			#if !UE_BUILD_SHIPPING
			if (AvoidVisionCvars::AvoidVisionDebug)
			{
				DrawDebugSphere(GetWorld(), LastAvoidancePlayerlocation, 60, 12, FColor::Cyan, true, 1.0f, 1, 2);
			}
			#endif
			
			bUsingAvoidancePath = true;
			CurrentPath = AvoidancePath;
			StartAvoidanceUpdates();
			UE_LOG(LogTemp, Warning, TEXT("::RequestMove Avoidance MetaPath successfully created"));
			return Super::RequestMove(RequestData, AvoidancePath);
		}
	}
	return Super::RequestMove(RequestData, InPath);
}

void UCustomPathFollowingComponent::OnPathFinished(const FPathFollowingResult& Result)
{
	Super::OnPathFinished(Result);
	UE_LOG(LogTemp, Error, TEXT(">>> OnPathFinished : Stop AVoidance Updates"));
	StopAvoidanceUpdates();
}

void UCustomPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	// #if !UE_BUILD_SHIPPING
	// if (AvoidVisionCvars::AvoidVisionDebug)
	// {
	// 	DrawConeOfVision(PlayerCharacter->GetActorForwardVector(),PlayerCharacter->GetActorLocation(), false);
	// }
	//#endif
	// if (bUsingAvoidancePath || !CurrentPath.IsValid())
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("[FollowPathSegment] bUsingAvoidancePath is true or CurrentPath is invalid"));
	// 	return;
	// }
	// if (IsPlayerCrossing(CurrentPath))
	// {
	// 	FNavPathSharedPtr AvoidancePath;
	// 	if (CreateAvoidanceMetaPath(CurrentPath, AvoidancePath))
	// 	{
	// 		LastAvoidancePlayerlocation = PlayerCharacter->GetActorLocation();
	// 		bUsingAvoidancePath = true;
	// 		CurrentPath = AvoidancePath;
	// 		StartAvoidanceUpdates();
	// 		UE_LOG(LogTemp, Warning, TEXT("[FollowPathSegment] Avoidance MetaPath successfully created"));
	// 		RequestMove(CurrentMoveRequest, CurrentPath);
	// 		return;
	// 	}
	// }
	Super::FollowPathSegment(DeltaTime);
}

bool UCustomPathFollowingComponent::IsPlayerCrossing(FNavPathSharedPtr InPath)
{
	UE_LOG(LogTemp, Error, TEXT(">>> IsPlayerCrossing"));
	
	AIController = Cast<AAIController>(GetOwner());
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(),0);
	if (!PlayerCharacter || !InPath.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[IsPlayerCrossing] Player OR Path are NOT Valid"));
		return false;
	}
	const TArray<FNavPathPoint>& Points = InPath->GetPathPoints();
	if (Points.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IsPlayerCrossing] Points less than 2"));
		return false;
	}
	const FVector PlayerLoc = PlayerCharacter->GetActorLocation();
	// Convertir a un cono de 45.f grados
	// 		ángulo      Dot
	//		0°          1.0
	//		45°         0.707
	//		90°         0.0
	//		180°       -1.0
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();

	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		DrawConeOfVision(PlayerForward,PlayerLoc, true);
	}
	#endif

	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		#if !UE_BUILD_SHIPPING
		if (AvoidVisionCvars::AvoidVisionDebug)
		{
			const FVector LocOffset = FVector(0, 0, 10);
			DrawDebugLine(GetWorld(),Points[i].Location + LocOffset,Points[i + 1].Location + 
				LocOffset,FColor::White,false,5.0f,SDPG_Foreground,8.0f);
		}
		#endif
		
		const FVector Start = Points[i].Location;
		const FVector End = Points[i + 1].Location;
		
		if (SegmentIntersectsVisionCone2D(Start, End, PlayerLoc, PlayerForward))
		{
			UE_LOG(LogTemp, Warning, TEXT("[IsPlayerCrossing] PATH INTERSECTS PLAYER VISION CONE"));
			return true;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[IsPlayerCrossing] NOT INTERSECTS"));
	return false;
}

bool UCustomPathFollowingComponent::SegmentIntersectsVisionCone2D(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& PlayerLocation, const FVector& PlayerForward)
{
	UE_LOG(LogTemp, Error, TEXT(">>> SegmentIntersectsVisionCone2D"));
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		const FVector Offset = FVector(0, 0, 30);
		DrawDebugLine(GetWorld(), SegmentStart + Offset,SegmentEnd + Offset, FColor::Red, false, 1.0f, 1, 8.0f);
		DrawDebugSphere(GetWorld(), SegmentStart + Offset, 15.f, 8, FColor::Cyan, false, 1.0f);
		DrawDebugSphere(GetWorld(), SegmentEnd + Offset, 15.f, 8, FColor::Cyan, false, 1.0f);
	}
	#endif
	
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
		if (DistanceSquared <= KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SegmentIntersectsVisionCone2D] DistanceSquared almost 0"));
			return true;
		}
		const FVector2D Direction = ToPoint.GetSafeNormal();
		const float Dot = FVector2D::DotProduct(Forward, Direction);
		return Dot >= CosHalfAngle;
	};

	if (IsPointInsideCone(Start))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SegmentIntersectsVisionCone2D] IsPointInsideCone -> Start"));
		CurrentIntersection = Start;
		return true;
	}
	if (IsPointInsideCone(End))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SegmentIntersectsVisionCone2D] IsPointInsideCone -> End"));
		CurrentIntersection = End;
		return true;
	}
	
	// Interseccion segmento / circulo : funcion cuadratica : A·t² + B·t + C = 0 (t es la posicion a lo largo del segmento)
	/// Posicion inicial del segmento - la posicion del player
	const FVector2D StartToPlayer = Start - Player;
	/// Segment = vector desde Start a End .
	/// Esto es la longitud del segmento al cuadrado.
	/// La necesitamos al cuadrado para poder evitar la raiz cuadrada
	const float A = FVector2D::DotProduct(Segment, Segment);
	/// si la longitud es practicamente Cero , evitamos division por cero
	if (A < KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SegmentIntersectsVisionCone2D] A is ZERO"));
		return false;
	}
	
	const float B = 2.f * FVector2D::DotProduct(StartToPlayer, Segment);
	/// distancia entre player y start al cuadrado - ConeDistance al cuadrado .
	const float C = FVector2D::DotProduct(StartToPlayer, StartToPlayer) - FMath::Square(ConeDistance);
	/// Discriminante de la cuadratica : Δ = B² - 4AC
	/// Su resultado dice como interactua la recta con el circulo.
	const float Discriminant = B * B - 4.0f * A * C;
	/// si el discriminante es negativo entonces no hay intereseccion.
	/// si el discriminante es 0, hay una interseccion tangencial.
	///					 ●
	///				   /  \
	///				  /    \
	///	  			 |     |
	///   Start ●────|─────|────● End
	if (Discriminant < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SegmentIntersectsVisionCone2D] Discriminant is less than ZERO"));
		return false;
	}
	/// Si discriminante es mayor a 0 , hay dos puntos de puntos de interseccion.
	/// Formula cuadratica : t = (-B ± √Δ) / (2A)
	const float SqrtDiscriminant = FMath::Sqrt(Discriminant);
	/// Inversa de 2A = una optimizacion para evitar hacer la division dos veces 
	const float Inv2A = 1.0f / (2.0f * A);
	/// en cambio podemos hacer dos multiplicaciones en su lugar.
	/// T1 = entrada al circulo . 
	/// Primera solucion = t₁ = (-B - √Δ) / (2A)
	const float T1 = (-B - SqrtDiscriminant) * Inv2A;
	/// T2 = salida del circulo .
	/// Segunda solucion = t₂ = (-B + √Δ) / (2A)
	const float T2 = (-B + SqrtDiscriminant) * Inv2A;
	/// Normalmente T1 < T2
	///////////////
	
	// Check points where the segment enters and out of circle
	if (T1 >= 0.0f && T1 <= 1.0f)
	{
		const FVector2D Intersection = Start + Segment * T1;
		if (IsPointInsideCone(Intersection))
		{
			#if !UE_BUILD_SHIPPING
			if (AvoidVisionCvars::AvoidVisionDebug)
			{
				const FVector2D ToIntersection = Intersection - Player;
				const float Distance = ToIntersection.Size();
				const FVector2D Direction = ToIntersection.GetSafeNormal();
				const float Dot = FVector2D::DotProduct(Forward, Direction);
				const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
				UE_LOG(LogTemp, Warning, TEXT("[VISION] T1 = %.4f | Point=(%.2f, %.2f) | Distance=%.2f | Dot=%.4f | Angle=%.2f"),
					T1, Intersection.X, Intersection.Y, Distance, Dot, Angle);
			}
			#endif
			CurrentIntersection = Intersection;
			return true;
		}
	}
	if (T2 >= 0.0f && T2 <= 1.0f)
	{
		const FVector2D Intersection = Start + Segment * T2;
		if (IsPointInsideCone(Intersection))
		{
			UE_LOG(LogTemp, Warning, TEXT("[VISION] Intersection T2"));
			CurrentIntersection = Intersection;
			return true;
		}
	}
	return false;
}

bool UCustomPathFollowingComponent::HandlePathUpdateEvent()
{
	UE_LOG(LogTemp, Error, TEXT(">>> HandlePathUpdateEvent"));
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		if (Path.IsValid())
		{
			const TArray<FNavPathPoint>& Points = Path->GetPathPoints();
			UE_LOG(LogTemp, Warning, TEXT("[PATH] | Points: %d | Partial: %s"), Path->GetPathPoints().Num(), Path->IsPartial() ? TEXT("TRUE") : TEXT("FALSE"));

			for (int32 i = 0; i < Points.Num(); ++i)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PATH] Point[%d] = %s"), i, *Points[i].Location.ToString());
				if (i < Points.Num() - 1)
				{
					DrawDebugLine(GetWorld(), Points[i].Location + 
						FVector(0, 0, 20), Points[i + 1].Location + 
						FVector(0, 0, 20), FColor::Yellow, false, 5.0f, 1, 5.0f);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[PATH] Path INVALID"));
		}
	}
	#endif
	return Super::HandlePathUpdateEvent();
}

bool UCustomPathFollowingComponent::CreateAvoidanceMetaPath(const FNavPathSharedPtr& OriginalPath, FNavPathSharedPtr& OutMetaPath)
{
	UE_LOG(LogTemp, Error, TEXT(">>> Create AVoidance MetaPath"));
	if (!OriginalPath.IsValid())
	{
		return false;
	}
	const TArray<FNavPathPoint>& Points = OriginalPath->GetPathPoints();
	if (Points.Num() < 2)
	{
		return false;
	}
	// TODO : ver esto.
	if (!PlayerCharacter)
	{
		PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}
	
	const FVector PlayerLoc = PlayerCharacter->GetActorLocation();
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
	const FVector Start = Points[0].Location; 
	FVector AvoidPoint;
	FVector AvoidPoint2;
	if (!GetAvoidanceWaypoint(Start, PlayerLoc, PlayerForward,AvoidPoint, AvoidPoint2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateAvoidanceMetaPath] Failed To get Avoidance points")); 
		return false;
	}
	
	TArray<FVector> Waypoints;
	Waypoints.Reserve(4);
	Waypoints.Add(Start);
	Waypoints.Add(AvoidPoint);
	if (AvoidPoint2 != FVector::ZeroVector)
	{
		Waypoints.Add(AvoidPoint2);
	}
	Waypoints.Add(CurrentPathGoal);

	// TODO : ver esto. AIController ? nullptr ? 
	TSharedPtr<FMetaNavMeshPath, ESPMode::ThreadSafe> MetaPath = MakeShared<FMetaNavMeshPath>(Waypoints, *AIController);
	if (!MetaPath.IsValid())
	{
		return false;
	}
	OutMetaPath = MetaPath;
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] META PATH CREATED: Start=%s Avoid=%s Goal=%s"), 
			*Start.ToString(), *AvoidPoint.ToString(), *CurrentPathGoal.ToString());
		const FVector Offset = FVector(0, 0, 20);
		DrawDebugLine(GetWorld(), Start + Offset, AvoidPoint + Offset, FColor::Yellow, false, 5.0f, 1, 10.0f);
		DrawDebugLine(GetWorld(), AvoidPoint + Offset, CurrentPathGoal + Offset, FColor::Yellow, false, 5.0f, 1, 10.0f);
	}
	#endif
	return true;
}

/// Cual es el desplazamiento minimo para que el segmento deje de cruzar el cono de vision ? 
bool UCustomPathFollowingComponent::GetAvoidanceWaypoint(const FVector& Start, const FVector& PlayerLocation, const FVector& PlayerForward,
	FVector& OutAvoidPoint, FVector& OutAvoidPoint2)
{
	UE_LOG(LogTemp, Error, TEXT(">>> Get Avoidance Waypoint"));
	
	const FVector NormalizedForward = PlayerForward.GetSafeNormal2D();
    if (NormalizedForward.IsNearlyZero())
    {
    	UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] NormalizedForward equal ZERO"));
	    return false;
    }
	/// un vector perpendicular al forward en el plano XY (seria el vector Right respecto a la direccion de avance)
    const FVector Right = FVector::CrossProduct(FVector::UpVector, NormalizedForward).GetSafeNormal2D();
    const FVector ToEnemy = Start - PlayerLocation;
    const float ForwardDistance = FVector::DotProduct(ToEnemy, NormalizedForward);
	if (ForwardDistance <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because ForwarDistance Less or equal ZERO"));
		return false;
	}
    const float SideDistance = FVector::DotProduct(ToEnemy, Right);
	const float SideSign = SideDistance >= 0.0f ? 1.0f : -1.0f;
	
	// point to calculate to.
	const FVector Intersection3D = {CurrentIntersection.X,CurrentIntersection.Y,0.0f};
	const FVector ToIntersection = Intersection3D;
	const float Distance2D = ToIntersection.Size2D();
	/// a distance behind player
	const float AdvanceDistance = FMath::Clamp(ForwardDistance,ForwardDistance * 1.1,ForwardDistance * 2.0);
	const float TargetForwardDistance = -ForwardDistance + AdvanceDistance;
	/// ancho del cono
    const float HalfAngleRadians = FMath::DegreesToRadians(HalfVisionCone);
	/// al momento es la mitad de la amplitud del cono = 770 (45%)
	const float ConeHalfWidth = HalfAngleRadians * Distance2D;
	
	/// Waypoint 
	const FVector LateralOffset = Right * ConeHalfWidth * SideSign;
    const FVector NodePoint = PlayerLocation - NormalizedForward  * TargetForwardDistance + LateralOffset;
	const UNavigationSystemV1* NavSys = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	if (!NavSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because NavSys invalid"));
		return false;
	}
	FNavLocation Projected;

	if (!NavSys->ProjectPointToNavigation(NodePoint, Projected))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because NavSys failed to project"));
		return false;
	}
	OutAvoidPoint = Projected.Location;
	
	/// second node if needed!
	// if NodePoint cross in front of the playerLocation + normalizedForward entonces necesito un Nodepoint2 por detras del player y proyectarlo como OutAvoidPoint2
	const FVector ToGoal = (CurrentPathGoal - NodePoint).GetSafeNormal();
	const float ForwardSide = FVector::DotProduct(ToGoal,NormalizedForward);
	UE_LOG(LogTemp,Warning,TEXT("[GetAvoidanceWaypoint] ForwardSide %f"),ForwardSide);
	
	DrawDebugSphere(GetWorld(), CurrentPathGoal, 35.0f, 16, FColor::Emerald, true, 10.0f, 1, 4.0f);
	DrawDebugLine(GetWorld(), NodePoint, CurrentPathGoal, FColor::Red, true, 15.0f, 1, 6.0f);
	// quiere decir que NodePoint se va a cruzar.. creamos un nodo mas
	if (ForwardSide > 0.0f)
	{
		FVector NodePoint2 = NodePoint - NormalizedForward * TargetForwardDistance - LateralOffset;
		FNavLocation Projected2;
		if (NavSys->ProjectPointToNavigation(NodePoint2, Projected2))
		{
			OutAvoidPoint2 = Projected2.Location;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because NavSys failed to project NODE 2"));
		}
	}
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		DrawDebugSphere(GetWorld(), NodePoint, 35.0f, 16, FColor::Emerald, false, 10.0f, 1, 4.0f);
		DrawDebugSphere(GetWorld(), OutAvoidPoint, 35.0f, 16, FColor::Yellow, false, 10.0f, 1, 4.0f);
		if (OutAvoidPoint2 != FVector::ZeroVector)
		{
			DrawDebugSphere(GetWorld(), OutAvoidPoint2, 75.0f, 16, FColor::Orange, false, 10.0f, 1, 4.0f);
		}
		const FVector Offset = FVector(0, 0, 20);
		DrawDebugLine(GetWorld(), Start + Offset, OutAvoidPoint + Offset, FColor::Yellow, false, 5.0f, 1, 6.0f);
	
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] ForwardDistance=%.2f SideDistance=%.2f ConeHalfWidth=%.2f"), ForwardDistance, SideDistance, ConeHalfWidth);
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] AvoidPoint = %s"), *OutAvoidPoint.ToString());
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] AvoidPoint2 = %s"), *OutAvoidPoint2.ToString());
	}
	#endif
    return true;
}

void UCustomPathFollowingComponent::StartAvoidanceUpdates()
{
	UE_LOG(LogTemp, Error, TEXT(">>> Start AVoidance Updates"));
	
	if (GetWorld()->GetTimerManager().IsTimerActive(AvoidanceUpdateTH)) return;
	GetWorld()->GetTimerManager().SetTimer(AvoidanceUpdateTH,this,
		&UCustomPathFollowingComponent::UpdateAvoidancePath,UpdateAvoidancePathRate,true);
}

void UCustomPathFollowingComponent::StopAvoidanceUpdates()
{
	UE_LOG(LogTemp, Error, TEXT(">>> Stop AVoidance Updates"));
	
	GetWorld()->GetTimerManager().ClearTimer(AvoidanceUpdateTH);
	bUsingAvoidancePath = false;
}
void UCustomPathFollowingComponent::UpdateAvoidancePath()
{
	UE_LOG(LogTemp, Error, TEXT(">>> Updating AVoidance"));
	if (!bUsingAvoidancePath) return;

	if (!AIController || !PlayerCharacter)
	{
		StopAvoidanceUpdates();
		return;
	}
	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	const float PlayerMovementSquared = FVector::DistSquared2D(PlayerLocation,LastAvoidancePlayerlocation);
	if (PlayerMovementSquared  < FMath::Square(MinPlayerMovementThreshold)) return;
	LastAvoidancePlayerlocation = PlayerLocation;
	
	const FVector EnemyLocation = AIController->GetPawn()->GetActorLocation();
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
	FVector AvoidPoint;
	FVector AvoidPoint2;
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		DrawConeOfVision(PlayerForward,PlayerCharacter->GetActorLocation(), false,1.f);
	}
	#endif
	if (!GetAvoidanceWaypoint(EnemyLocation,LastAvoidancePlayerlocation,PlayerForward,AvoidPoint, AvoidPoint2))
	{
		UE_LOG(LogTemp, Error, TEXT("[UpdateAvoidancePath] Cant Get GetAvoidanceWaypoint"));
		return;
	}
	
	TArray<FVector> Waypoints;
	Waypoints.Reserve(4);
	Waypoints.Add(EnemyLocation);
	Waypoints.Add(AvoidPoint);
	if (AvoidPoint2 != FVector::ZeroVector)
	{
		Waypoints.Add(AvoidPoint2);
	}
	Waypoints.Add(CurrentPathGoal);

	const TSharedPtr<FMetaNavMeshPath,ESPMode::ThreadSafe> MetaPath = MakeShared<FMetaNavMeshPath>(Waypoints,*AIController);
	if (!MetaPath.IsValid())
	{
		return;
	}
	
	if (!IsPlayerCrossing(CurrentPath))
	{
		StopAvoidanceUpdates();
		RequestMove(CurrentMoveRequest, MetaPath);
	}
}

#if !UE_BUILD_SHIPPING
void UCustomPathFollowingComponent::DrawConeOfVision(const FVector& PlayerForwardVector, 
					const FVector& PlayerLocation, bool bPersistentLines, float LifeTime) const
{
	const FVector LeftBoundary = PlayerForwardVector.RotateAngleAxis(-HalfVisionCone, FVector::UpVector);
	const FVector RightBoundary = PlayerForwardVector.RotateAngleAxis(HalfVisionCone, FVector::UpVector);
	const FVector EndLeftWhisker = PlayerLocation + (LeftBoundary * ConeDistance);
	const FVector EndRightWhisker = PlayerLocation + (RightBoundary * ConeDistance);
	const FVector EndPlayerVision = PlayerLocation + (PlayerForwardVector * ConeDistance);

	DrawDebugSphere(GetWorld(), PlayerLocation, 60, 12, FColor::Blue, bPersistentLines, LifeTime, 1, 2);
	DrawDebugLine(GetWorld(),PlayerLocation,EndPlayerVision,FColor::Blue,bPersistentLines,LifeTime,1,3.0f);
	DrawDebugLine(GetWorld(),PlayerLocation,EndLeftWhisker,FColor::Green,bPersistentLines,LifeTime,1,4.0f);
	DrawDebugLine(GetWorld(),PlayerLocation,EndRightWhisker,FColor::Green,bPersistentLines,LifeTime,1,4.0f);
	DrawDebugLine(GetWorld(),EndLeftWhisker,EndRightWhisker,FColor::Green,bPersistentLines,LifeTime,1,4.0f);
}
#endif



