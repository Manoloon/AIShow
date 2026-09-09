
#include "Misc/AutomationTest.h"
#include "CustomPathFollowingComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCustomPathFollowingComponent_SegmentIntersectsVisionCone2D,
	"CustomPathFollowingComponent.SegmentIntersectsVisionCone2D",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

bool FCustomPathFollowingComponent_SegmentIntersectsVisionCone2D::RunTest(const FString& Parameters)
{
	UCustomPathFollowingComponent* Comp = NewObject<UCustomPathFollowingComponent>();
	const FVector PlayerLocation = FVector::ZeroVector;
	const FVector PlayerForward = FVector(1.f,0.f,0.f);
	
	// Test 1 : Segmento directamente en frente del player a distancia igual al cone distance
	{
		// 600 es el cone distance
		const FVector SegmentStart = FVector(600.f,-50.f,0.f);
		const FVector SegmentEnd = FVector(600.f,50.f,0.f);
		
		const bool bIntersects = Comp->SegmentIntersectsVisionCone2D(SegmentStart,
			SegmentEnd,PlayerLocation,PlayerForward);
		
		TestTrue(TEXT("Un segmento en frente del player a distancia igual al cone distance,debe intersectar con el cono de vision"),
			bIntersects);
	}
	// Test 2 : Segmento directamente detras del player
	{
		const FVector SegmentStart = FVector(-300.f,-50.f,0.f);
		const FVector SegmentEnd = FVector(-300.f,50.f,0.f);
		
		const bool bIntersects = Comp->SegmentIntersectsVisionCone2D(SegmentStart,
			SegmentEnd,PlayerLocation,PlayerForward);
		
		TestFalse(TEXT("Un segmento detras del player NO debe intersectar con el cono de vision"),
			bIntersects); 
	}
	// Test 3 : Segmento fuera del cono de vision
	{
		const FVector SegmentStart = FVector(300.f,300.f,0.f);
		const FVector SegmentEnd = FVector(400.f,400.f,0.f);
		
		const bool bIntersects = Comp->SegmentIntersectsVisionCone2D(SegmentStart,
			SegmentEnd,PlayerLocation,PlayerForward);
		
		TestFalse(TEXT("Un segmento fuera del cono de vision NO debe intersectar"),
			bIntersects); 
	}
	// Test 4 : Player mirando en la direccion opuesta
	{
		const FVector SegmentStart = FVector(300.f,-50.f,0.f);
		const FVector SegmentEnd = FVector(300.f,50.f,0.f);
		const FVector OppositeForward = FVector(-1.f,0.f,0.f);
		const bool bIntersects = Comp->SegmentIntersectsVisionCone2D(SegmentStart,
			SegmentEnd,PlayerLocation,OppositeForward);
		
		TestFalse(TEXT("Un segmento detras del player forward vector NO debe intersectar con el cono de vision"),
			bIntersects); 
	}
	// Test 5 : Z no debe afectar a la interseccion con el cono 
	{
		const FVector SegmentStart = FVector(600.f,-50.f,100.f);
		const FVector SegmentEnd = FVector(600.f,50.f,100.f);
		
		const bool bIntersects = Comp->SegmentIntersectsVisionCone2D(SegmentStart,
			SegmentEnd,PlayerLocation,PlayerForward);
		
		TestTrue(TEXT("El eje Z no debe afectar la interseccion con el cono de vision"),
			bIntersects); 
	}
	// Test 6 : Segmento directamente en frente del player a distancia menor al cone distance
	{
		// 600 es el cone distance
		const FVector SegmentStart = FVector(600.f,-350.f,0.f);
		const FVector SegmentEnd = FVector(600.f,1 50.f,0.f);
		
		const bool bIntersects = Comp->SegmentIntersectsVisionCone2D(SegmentStart,
			SegmentEnd,PlayerLocation,PlayerForward);
		
		TestTrue(TEXT("Un segmento en frente del player a distancia menor al cone distance,debe intersectar con el cono de vision"),
			bIntersects);
	}
	// Test 6 : Segmento directamente en frente del player a distancia Mayor al cone distance
	{
		// 600 es el cone distance
		const FVector SegmentStart = FVector(200.f,-350.f,0.f);
		const FVector SegmentEnd = FVector(900.f,250.f,0.f);
		
		const bool bIntersects = Comp->SegmentIntersectsVisionCone2D(SegmentStart,
			SegmentEnd,PlayerLocation,PlayerForward);
		
		TestTrue(TEXT("Un segmento en frente del player a distancia Mayor al cone distance,debe intersectar con el cono de vision"),
			bIntersects);
	}
	return true;
}
