
#include "AIController.h"
#include "Misc/AutomationTest.h"
#include "CustomPathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "Navigation/MetaNavMeshPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCustomPathFollowingComponent_IsPlayerCrossing,
	"CustomPathFollowingComponent.IsPlayerCrossing",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

// helper 
FNavPathSharedPtr CreateTestPath(const TArray<FVector>& Locations)
{
	return MakeShared<FNavigationPath>(Locations);
}

bool FCustomPathFollowingComponent_IsPlayerCrossing::RunTest(const FString& Parameters)
{
	UCustomPathFollowingComponent* Comp = NewObject<UCustomPathFollowingComponent>(NewObject<AAIController>());
	Comp->Initialize();
	const FVector PlayerLocation = FVector{0.f,0.f,0.f};
	const FVector PlayerForward = FVector(1.f,0.f,0.f);
	const FVector TargetFrenteAlPlayer = {730.f,10.f,0.f};
	const FVector TargetAtrasDerecha = {130,290,0};
	const FVector TargetAtrasIzquierda = {-580,-480,0};
	
	// Test 1 : InPath esta vacio.
	{
		FNavPathSharedPtr Path;
		const bool bPathIsInvalid = Comp->IsPlayerCrossing(Path, PlayerLocation, PlayerForward);
		UE_LOG(LogTemp,Warning,TEXT("bPathIsInvalid : %hs"),bPathIsInvalid ? "TRUE" : "FALSE");
        TestFalse(TEXT("El Path esta vacio o es invalido"),bPathIsInvalid);
	}
	// Test 2 : InPath es Valido y Cruzamos desde izquierda a derecha al player
	{
		const FNavPathSharedPtr Path = CreateTestPath({	
		FVector(1010.f,340.f,0.f),TargetAtrasDerecha});
		Path->MarkReady();
		const bool bIsPlayerCrossing = Comp->IsPlayerCrossing(Path, PlayerLocation, PlayerForward);
		TestTrue(TEXT("El Path es valido y Cruzamos desde izquierda a derecha al player"),bIsPlayerCrossing);
	}
	// Test 3 : No cruzamos al player
	{
		const FNavPathSharedPtr Path = CreateTestPath({	
		FVector(1000.f,-10.f,0.f),TargetFrenteAlPlayer});
		Path->MarkReady();
		const bool bIsPlayerCrossing = Comp->IsPlayerCrossing(Path, PlayerLocation, PlayerForward);
		TestFalse(TEXT("No cruzamos al player"),bIsPlayerCrossing);
	}
	// Test 4 : Cruzamos al player desde Derecha a Izquierda
	{
		const FNavPathSharedPtr Path = CreateTestPath({	
		FVector(1000.f,-10.f,0.f),TargetAtrasIzquierda});
		Path->MarkReady();
		const bool bIsPlayerCrossing = Comp->IsPlayerCrossing(Path, PlayerLocation, PlayerForward);
		TestTrue(TEXT("Cruzamos al player desde Derecha a Izquierda"),bIsPlayerCrossing);
	}
    return true;
}