// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCrowdManager.h"

namespace FCrowdDebug
{
	/** if set, debug information will be displayed for agent selected in editor */
	int32 DebugSelectedActors = 1;
	FAutoConsoleVariableRef CVarDebugSelectedActors(TEXT("ai.crowd.DebugSelectedActors"), DebugSelectedActors,
		TEXT("Enable debug drawing for selected crowd agent.\n0: Disable, 1: Enable"), ECVF_Default);

	/** if set, basic debug information will be recorded in VisLog for all agents */
	int32 DebugVisLog = 1;
	FAutoConsoleVariableRef CVarDebugVisLog(TEXT("ai.crowd.DebugVisLog"), DebugVisLog,
		TEXT("Enable detailed vislog recording for all crowd agents.\n0: Disable, 1: Enable"), ECVF_Default);

	/** debug flags, works only for selected actor */
	int32 DrawDebugCorners = 1;
	FAutoConsoleVariableRef CVarDrawDebugCorners(TEXT("ai.crowd.DrawDebugCorners"), DrawDebugCorners,
		TEXT("Draw path corners data, requires ai.crowd.DebugSelectedActors.\n0: Disable, 1: Enable"), ECVF_Default);
	
	int32 DrawDebugCollisionSegments = 1;
	FAutoConsoleVariableRef CVarDrawDebugCollisionSegments(TEXT("ai.crowd.DrawDebugCollisionSegments"), DrawDebugCollisionSegments,
		TEXT("Draw colliding navmesh edges, requires ai.crowd.DebugSelectedActors.\n0: Disable, 1: Enable"), ECVF_Default);
	
	int32 DrawDebugPath = 1;
	FAutoConsoleVariableRef CVarDrawDebugPath(TEXT("ai.crowd.DrawDebugPath"), DrawDebugPath,
		TEXT("Draw active paths, requires ai.crowd.DebugSelectedActors.\n0: Disable, 1: Enable"), ECVF_Default);
	
	int32 DrawDebugVelocityObstacles = 1;
	FAutoConsoleVariableRef CVarDrawDebugVelocityObstacles(TEXT("ai.crowd.DrawDebugVelocityObstacles"), DrawDebugVelocityObstacles,
		TEXT("Draw velocity obstacle sampling, requires ai.crowd.DebugSelectedActors.\n0: Disable, 1: Enable"), ECVF_Default);
	
	int32 DrawDebugPathOptimization = 1;
	FAutoConsoleVariableRef CVarDrawDebugPathOptimization(TEXT("ai.crowd.DrawDebugPathOptimization"), DrawDebugPathOptimization,
		TEXT("Draw path optimization data, requires ai.crowd.DebugSelectedActors.\n0: Disable, 1: Enable"), ECVF_Default);
	
	int32 DrawDebugNeighbors = 1;
	FAutoConsoleVariableRef CVarDrawDebugNeighbors(TEXT("ai.crowd.DrawDebugNeighbors"), DrawDebugNeighbors,
		TEXT("Draw current neighbors data, requires ai.crowd.DebugSelectedActors.\n0: Disable, 1: Enable"), ECVF_Default);

	/** debug flags, don't depend on agent */
	int32 DrawDebugBoundaries = 1;
	FAutoConsoleVariableRef CVarDrawDebugBoundaries(TEXT("ai.crowd.DrawDebugBoundaries"), DrawDebugBoundaries,
		TEXT("Draw shared navmesh boundaries used by crowd simulation.\n0: Disable, 1: Enable"), ECVF_Default);

	const FVector Offset(0, 0, 20);

	const FColor Corner(128, 0, 0);
	const FColor CornerLink(192, 0, 0);
	const FColor CornerFixed(192, 192, 0);
	const FColor CollisionRange(192, 0, 128);
	const FColor CollisionSeg0(192, 0, 128);
	const FColor CollisionSeg1(96, 0, 64);
	const FColor CollisionSegIgnored(128, 128, 128);
	const FColor Path(255, 255, 255);
	const FColor PathSpecial(255, 192, 203);
	const FColor PathOpt(0, 128, 0);
	const FColor AvoidanceRange(255, 255, 255);
	const FColor Neighbor(0, 192, 128);

	const float LineThickness = 3.f;
}