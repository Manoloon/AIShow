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
	UE_LOG(LogTemp, Warning, TEXT(">>> Request Move"));
		
	if (!InPath.IsValid())
	{
		UE_LOG(LogTemp, Warning,TEXT("::RequestMove Path NOT valid -> PASS THROUGH"));
		return Super::RequestMove(RequestData, InPath);
	}
	if (!PlayerCharacter)
	{
		PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(),0);
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
	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
	if (IsPlayerCrossing(InPath, PlayerLocation, PlayerForward))
	{
		FNavPathSharedPtr AvoidancePath;
		if (CreateAvoidanceMetaPath(InPath, PlayerLocation, PlayerForward, AvoidancePath))
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
	UE_LOG(LogTemp, Warning, TEXT(">>> OnPathFinished : Stop AVoidance Updates"));
	StopAvoidanceUpdates();
}

void UCustomPathFollowingComponent::Initialize()
{
	Super::Initialize();
	AIController = Cast<AAIController>(GetOwner());
}

bool UCustomPathFollowingComponent::IsPlayerCrossing(const FNavPathSharedPtr& InPath, const FVector& PlayerLocation, const FVector& PlayerForwardVector)
{
	if (!InPath.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[IsPlayerCrossing] Path is NOT Valid"));
		return false;
	}
	UE_LOG(LogTemp, Display, TEXT(">>> IsPlayerCrossing"));
	const TArray<FNavPathPoint>& Points = InPath->GetPathPoints();
	if (Points.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IsPlayerCrossing] Points less than 2"));
		return false;
	}
	// Convertir a un cono de 45.f grados
	// 		ángulo      Dot
	//		0°          1.0
	//		45°         0.707
	//		90°         0.0
	//		180°       -1.0
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		DrawConeOfVision(PlayerForwardVector,PlayerLocation, true);
	}
	#endif

	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		#if !UE_BUILD_SHIPPING
		if (AvoidVisionCvars::AvoidVisionDebug)
		{
			DrawDebugLine(GetWorld(),Points[i].Location + VertOffset,Points[i + 1].Location + 
				VertOffset,FColor::White,false,5.0f,SDPG_Foreground,8.0f);
		}
		#endif
		
		const FVector Start = Points[i].Location;
		const FVector End = Points[i + 1].Location;
		
		if (SegmentIntersectsVisionCone2D(Start, End, PlayerLocation, PlayerForwardVector))
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
	UE_LOG(LogTemp, Display, TEXT(">>> SegmentIntersectsVisionCone2D"));
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		DrawDebugLine(GetWorld(), SegmentStart + VertOffset,SegmentEnd + VertOffset, FColor::Red, false, 1.0f, 1, 8.0f);
		DrawDebugSphere(GetWorld(), SegmentStart + VertOffset, 15.f, 8, FColor::Cyan, false, 1.0f);
		DrawDebugSphere(GetWorld(), SegmentEnd + VertOffset, 15.f, 8, FColor::Cyan, false, 1.0f);
	}
	#endif
	
	const FVector2D Start(SegmentStart.X, SegmentStart.Y);
	const FVector2D End(SegmentEnd.X, SegmentEnd.Y);
	const FVector2D Player(PlayerLocation.X, PlayerLocation.Y);
	FVector2D Forward(PlayerForward.X, PlayerForward.Y);
	if (!Forward.Normalize())
	{
		return false;
	}
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
		return Dot > CosHalfAngle;
	};
	
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
	UE_LOG(LogTemp, Display, TEXT(">>> HandlePathUpdateEvent"));
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
						VertOffset, Points[i + 1].Location + 
						VertOffset, FColor::Yellow, false, 5.0f, 1, 5.0f);
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

TArray<FVector> UCustomPathFollowingComponent::GetWaypoints(const FVector& Start, const TArray<FVector>& AvoidPoints) const
{
	TArray<FVector> Waypoints;
	Waypoints.Reserve(4);
	Waypoints.Add(Start);
	Waypoints.Add(AvoidPoints[0]);
	if (AvoidPoints.Num() > 1)
	{
		Waypoints.Add(AvoidPoints[1]);
	}
	Waypoints.Add(CurrentPathGoal);
	return Waypoints;
}
bool UCustomPathFollowingComponent::CheckIfTargetIsNearPlayer(const FVector& Start, const FVector& End,const FVector& NormalizedForward, float Radius) const
{
	const FVector ClosestPoint = FMath::ClosestPointOnSegment(PlayerCharacter->GetActorLocation(),Start,End);
	const FVector ToClosest = ClosestPoint - PlayerCharacter->GetActorLocation();
	const float DistanceSquared = ToClosest.SizeSquared();
	const float ForwardDistance = FVector::DotProduct(ToClosest,NormalizedForward);
	const bool bPassesNearPlayer = DistanceSquared <= FMath::Square(Radius);
	const bool bPassesInFrontPlayer = ForwardDistance > 0.0f;
	return bPassesNearPlayer && bPassesInFrontPlayer;
}

bool UCustomPathFollowingComponent::CreateAvoidanceMetaPath(const FNavPathSharedPtr& InPath, const FVector& PlayerLocation, const FVector& PlayerForwardVector, FNavPathSharedPtr& OutMetaPath)
{
	UE_LOG(LogTemp, Display, TEXT(">>> Create AVoidance MetaPath"));
	if (!InPath.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateAvoidanceMetaPath] Path is NOT valid"));
		return false;
	}
	const TArray<FNavPathPoint>& Points = InPath->GetPathPoints();
	if (Points.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateAvoidanceMetaPath] Points less than 2"));
		return false;
	}
	// Check si la distancia del player con respecto al goal es menor al ConeDistance , sino , no hacer ningun avoidance.
	const FVector Start = Points[0].Location;
	if (!SegmentIntersectsVisionCone2D(
		Start,
		CurrentPathGoal,
		PlayerLocation,
		PlayerForwardVector))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateAvoidanceMetaPath] The goal is outside ConeDistance"));
		return false;
	}
	TArray<FVector> AvoidancePoints;
	if (!GetAvoidancePoints(Start,PlayerLocation,PlayerForwardVector,AvoidancePoints))
	{
		UE_LOG(LogTemp, Error, TEXT("[UpdateAvoidancePath] Cant Get GetAvoidanceWaypoint projected"));
		return false;
	}
	TArray<FVector> Waypoints = GetWaypoints(Start, AvoidancePoints);
	
	TSharedPtr<FMetaNavMeshPath, ESPMode::ThreadSafe> MetaPath = MakeShared<FMetaNavMeshPath>(Waypoints, *AIController);
	if (!MetaPath.IsValid())
	{
		return false;
	}
	OutMetaPath = MetaPath;
	
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug && !AvoidancePoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AVOIDANCE] META PATH CREATED: Start=%s Avoid=%s Goal=%s"), 
			*Start.ToString(), *Waypoints[0].ToString(), *CurrentPathGoal.ToString());
		
		DrawDebugLine(GetWorld(), Start + VertOffset, Waypoints[0] + VertOffset, FColor::Yellow, false, 5.0f, 1, 10.0f);
		if (AvoidancePoints.Num() > 1)
		{
			DrawDebugLine(GetWorld(), AvoidancePoints[0] + VertOffset, AvoidancePoints[1] + VertOffset, FColor::Orange, false, 5.0f, 1, 10.0f);
			DrawDebugLine(GetWorld(), AvoidancePoints[1] + VertOffset, CurrentPathGoal + VertOffset, FColor::Yellow, false, 5.0f, 1, 10.0f);
		}
		else
		{
			DrawDebugLine(GetWorld(), AvoidancePoints[0] + VertOffset, CurrentPathGoal + VertOffset, FColor::Yellow, false, 5.0f, 1, 10.0f);
		}
	}
	#endif
	return true;
}

bool UCustomPathFollowingComponent::GetSecondAvoidanceWaypoint(const FVector& PreviousNodePoint, const FVector& NormalizedForward, const FVector& PrevLateralOffset, FVector& OutAvoidPoint) const
{
	const FVector ToGoal = (CurrentPathGoal - PreviousNodePoint).GetSafeNormal();
	const float ForwardDistance = FVector::DotProduct(ToGoal,NormalizedForward);
	/// buscamos una distancia por detras del jugador
	const float TargetForwardDistance = -ForwardDistance + ForwardDistance * 1.1;
	UE_LOG(LogTemp,Warning,TEXT("[GetAvoidanceWaypoint] ForwardSide %f"),ForwardDistance);
		
	// Solo si Node1 a Goal cruzaria por delante de player
	if (ForwardDistance > 0.0f)
	{
		const FVector NodePoint2 = PreviousNodePoint - NormalizedForward * TargetForwardDistance - PrevLateralOffset;
		FNavLocation Projected2;
		const UNavigationSystemV1* NavSys = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
		if (NavSys->ProjectPointToNavigation(NodePoint2, Projected2))
		{
			OutAvoidPoint = {Projected2.Location.X,Projected2.Location.Y,0.0f};
			#if !UE_BUILD_SHIPPING
			if (AvoidVisionCvars::AvoidVisionDebug)
			{
				DebugAvoidanceWaypoint(PreviousNodePoint, OutAvoidPoint, NodePoint2,TargetForwardDistance,true);
			}
			#endif
			return true;
		}
			OutAvoidPoint = FVector::ZeroVector;
			UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because NavSys failed to project NODE 2"));
		return false;
	}
	return false;
}

/// Cual es el desplazamiento minimo para que el segmento deje de cruzar el cono de vision ? 
bool UCustomPathFollowingComponent::GetFirstAvoidanceWaypoint(const FVector& Start, const FVector& PlayerLocation, const FVector& NormalizedForward,
	FVector& OutAvoidPoint, FVector& OutCreatedNode, FVector& OutLateralOffset) const
{
	UE_LOG(LogTemp, Display, TEXT(">>> Get Avoidance Waypoint"));
	/// un vector perpendicular al forward en el plano XY (seria el vector Right respecto a la direccion de avance)
    const FVector Right = FVector::CrossProduct(FVector::UpVector, NormalizedForward).GetSafeNormal2D();
    const FVector ToEnemy = Start - PlayerLocation;
    const float ForwardDistance = FVector::DotProduct(ToEnemy, NormalizedForward);
	if (ForwardDistance <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because ForwarDistance Less or equal ZERO"));
		return false;
	}
	
	/// buscamos una distancia por detras del jugador
	const float TargetForwardDistance = -ForwardDistance + ForwardDistance * 1.1;
	/// ancho del cono
    const float HalfAngleRadians = FMath::DegreesToRadians(HalfVisionCone);
	/// El punto con el cual hacer el calculo.
	/// Este dato lo hemos obtenido desde SegmentIntersectsVisionCone2D.	
	const FVector Intersection3D = {CurrentIntersection.X,CurrentIntersection.Y,0.0f};
	const float Distance2D = Intersection3D.Size2D();
	/// al momento es la mitad de la amplitud del cono = 770 (45%)
	const float ConeHalfWidth = HalfAngleRadians * Distance2D;
	
	/// Waypoint 
	/// Offset hacia un lado para evitar el cono de vision
	const float SideDistance = FVector::DotProduct(ToEnemy, Right);
	const float SideSign = SideDistance >= 0.0f ? 1.0f : -1.0f;
	OutLateralOffset = Right * ConeHalfWidth * SideSign;
    OutCreatedNode = PlayerLocation - NormalizedForward  * TargetForwardDistance + OutLateralOffset;
	
	const UNavigationSystemV1* NavSys = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	if (!NavSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because NavSys invalid"));
		return false;
	}
	FNavLocation Projected;

	if (!NavSys->ProjectPointToNavigation(OutCreatedNode, Projected))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] FALSE because NavSys failed to project"));
		return false;
	}
	OutAvoidPoint = {Projected.Location.X,Projected.Location.Y,0.0f};
	
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		DebugAvoidanceWaypoint(Start, OutAvoidPoint, OutCreatedNode, ForwardDistance, false,ConeHalfWidth, SideDistance);
	}
	#endif
    return true;
}

bool UCustomPathFollowingComponent::GetAvoidancePoints(const FVector& Start, const FVector& PlayerLocation, const FVector& PlayerForward, TArray<FVector>& OutAvoidancePoints) const
{
	FVector AvoidPoint  = FVector::ZeroVector;
	FVector NodeOneCreated = FVector::ZeroVector;
	FVector OutLateralOffset = FVector::ZeroVector;
	const FVector NormalizedForward = PlayerForward.GetSafeNormal2D();
	if (NormalizedForward.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetAvoidancePoints] NormalizedForward equal ZERO"));
		return false;
	}
	if (!GetFirstAvoidanceWaypoint(Start, PlayerLocation, NormalizedForward,
		AvoidPoint, NodeOneCreated, OutLateralOffset))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateAvoidanceMetaPath] Failed To get Avoidance points")); 
		return false;
	}
	OutAvoidancePoints.Add(AvoidPoint);
	if (CheckIfTargetIsNearPlayer(NodeOneCreated,CurrentPathGoal,NormalizedForward))
	{
		FVector AvoidPoint2 = FVector::ZeroVector;
		if (!GetSecondAvoidanceWaypoint(NodeOneCreated, NormalizedForward,OutLateralOffset, AvoidPoint2))
		{
			UE_LOG(LogTemp, Warning, TEXT("[CreateAvoidanceMetaPath] Failed To get the Second Avoidance point projected")); 
			return false;
		}
		OutAvoidancePoints.Add(AvoidPoint2);
	}
	return true;
}

void UCustomPathFollowingComponent::StartAvoidanceUpdates()
{
	UE_LOG(LogTemp, Display, TEXT(">>> Start AVoidance Updates"));
	
	if (GetWorld()->GetTimerManager().IsTimerActive(AvoidanceUpdateTH)) return;
	GetWorld()->GetTimerManager().SetTimer(AvoidanceUpdateTH,this,
		&UCustomPathFollowingComponent::UpdateAvoidancePath,UpdateAvoidancePathRate,true);
	UE_LOG(LogTemp, Display, TEXT(">>> Updating AVoidance"));
}

void UCustomPathFollowingComponent::StopAvoidanceUpdates()
{
	UE_LOG(LogTemp, Display, TEXT(">>> Stop AVoidance Updates"));
	GetWorld()->GetTimerManager().ClearTimer(AvoidanceUpdateTH);
	bUsingAvoidancePath = false;
}

void UCustomPathFollowingComponent::UpdateAvoidancePath()
{
	/// si no estamos en modo avoidance mejor no hacer ningun update ,no?
	if (!bUsingAvoidancePath) return;
	/// si estamos en dicho modo pero el agente ya no tiene controller o el player no existe pues , bye bye .
	if (!AIController || !PlayerCharacter)
	{
		StopAvoidanceUpdates();
		return;
	}
	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	const FVector EnemyLocation = AIController->GetPawn()->GetActorLocation();
	const FVector PlayerForward = PlayerCharacter->GetActorForwardVector();
	
	#if !UE_BUILD_SHIPPING
	if (AvoidVisionCvars::AvoidVisionDebug)
	{
		DrawConeOfVision(PlayerForward,PlayerLocation, false,UpdateAvoidancePathRate);
	}
	#endif
	
	const float PlayerMovementSquared = FVector::DistSquared2D(PlayerLocation,LastAvoidancePlayerlocation);
	if (PlayerMovementSquared  < FMath::Square(MinPlayerMovementThreshold))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UpdateAvoidancePath] The player hasnt move yet"));
		return;
	}
	// El jugador se ha movido lo suficiente para actualizar su localizacion
	LastAvoidancePlayerlocation = PlayerLocation;
	
	TArray<FVector> AvoidancePoints;
	if (!GetAvoidancePoints(EnemyLocation,LastAvoidancePlayerlocation,PlayerForward,AvoidancePoints))
	{
		UE_LOG(LogTemp, Error, TEXT("[UpdateAvoidancePath] Cant Get GetAvoidanceWaypoint projected"));
		return;
	}
	
	TArray<FVector> Waypoints = GetWaypoints(EnemyLocation,AvoidancePoints);

	const TSharedPtr<FMetaNavMeshPath,ESPMode::ThreadSafe> MetaPath = MakeShared<FMetaNavMeshPath>(Waypoints,*AIController);
	if (!MetaPath.IsValid())
	{
		return;
	}
	
	if (!IsPlayerCrossing(CurrentPath, PlayerLocation, PlayerForward))
	{
		StopAvoidanceUpdates();
		RequestMove(CurrentMoveRequest, MetaPath);
	}
}

#if !UE_BUILD_SHIPPING
void UCustomPathFollowingComponent::DebugAvoidanceWaypoint(const FVector& Start, const FVector& OutAvoidPoint, const FVector& NodePoint,const float ForwardDistance,
	bool IsSecondWaypoint, const float ConeHalfWidth, const float SideDistance) const
{
	DrawDebugSphere(GetWorld(), NodePoint, 35.0f, 16, FColor::Emerald, false, 10.0f, 1, 4.0f);
	DrawDebugSphere(GetWorld(), CurrentPathGoal, 35.0f, 16, FColor::Emerald, false, 10.0f, 1, 4.0f);
	DrawDebugLine(GetWorld(), NodePoint, CurrentPathGoal, FColor::Red, false, 15.0f, 1, 6.0f);
	
	if (IsSecondWaypoint)
	{
		DrawDebugSphere(GetWorld(), OutAvoidPoint, 75.0f, 16, FColor::Orange, false, 10.0f, 1, 4.0f);
	}
	else
	{
		DrawDebugSphere(GetWorld(), OutAvoidPoint, 35.0f, 16, FColor::Yellow, false, 10.0f, 1, 4.0f);
	}
	DrawDebugLine(GetWorld(), Start + VertOffset, OutAvoidPoint + VertOffset, FColor::Yellow, false, 5.0f, 1, 6.0f);
	
	UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] ForwardDistance=%.2f SideDistance=%.2f ConeHalfWidth=%.2f"), ForwardDistance, SideDistance, ConeHalfWidth);
	UE_LOG(LogTemp, Warning, TEXT("[GetAvoidanceWaypoint] AvoidPoint = %s"), *OutAvoidPoint.ToString());
}

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



