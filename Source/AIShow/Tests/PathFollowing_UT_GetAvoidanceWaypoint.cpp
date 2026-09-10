
#include "AIController.h"
#include "Misc/AutomationTest.h"
#include "CustomPathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "Navigation/MetaNavMeshPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCustomPathFollowingComponent_GetAvoidanceWaypoint,
	"CustomPathFollowingComponent.GetAvoidanceWaypoint",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

// helper 
FNavPathSharedPtr CreateTestPath(const TArray<FVector>& Locations)
{
	return MakeShared<FNavigationPath>(Locations);
}

bool FCustomPathFollowingComponent_GetAvoidanceWaypoint::RunTest(const FString& Parameters)
{
	UCustomPathFollowingComponent* Comp = NewObject<UCustomPathFollowingComponent>(NewObject<AAIController>());
	Comp->Initialize();
	const FVector PlayerLocation = FVector{0.f,0.f,0.f};
	const FVector PlayerForward = FVector(1.f,0.f,0.f);
	const FVector TargetFrenteAlPlayer = {730.f,10.f,0.f};
	const FVector TargetAtrasDerecha = {130,290,0};
	const FVector TargetAtrasIzquierda = {-580,-480,0};
	
	// Test 1 : Player Forward Vector Normalized is Not Zero
	// Test 2 : Distance from the Start - PlayerLocation to the normalized Forward is Not Zero or less
	// Test 3 : Navigation System is Valid 
	// Test 4 : Project the point to the navigation plan is valid 
	// Test 5 : Waypoint valid
    return true;
}