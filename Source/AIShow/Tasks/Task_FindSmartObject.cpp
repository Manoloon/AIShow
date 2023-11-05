// Fill out your copyright notice in the Description page of Project Settings.

#include "Task_FindSmartObject.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BlackboardKeyType_SOClaimHandle.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "SmartObjectSubsystem.h"

EBTNodeResult::Type
UTask_FindSmartObject::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	if (!IsValid(GetWorld()))
	{
		return EBTNodeResult::Failed;
	}
	USmartObjectSubsystem* SOSubSystem =
		GetWorld()->GetSubsystem<USmartObjectSubsystem>();
	AAIController* MyController = OwnerComp.GetAIOwner();
	ACharacter* Character = Cast<ACharacter>(MyController->GetPawn());
	if (!IsValid(SOSubSystem) || !IsValid(MyController) || !IsValid(Character))
	{
		return EBTNodeResult::Failed;
	}
	// Cast<AAIController>(OwnerComp.GetOwner());
	const FVector PawnLoc = Character->GetActorLocation();
	const FVector VectorMultip = { 2000, 2000, 2000 };
	const FVector MinZone = PawnLoc - VectorMultip;
	const FVector MaxZone = PawnLoc + VectorMultip;
	FBox Zone = { MinZone, MaxZone };
	DrawDebugBox(GetWorld(), PawnLoc, MinZone + MaxZone, FColor::Magenta, false, 1, 1, 2);
	FSmartObjectRequest Request;
	Request.QueryBox = Zone; // Request.QueryBox.ShiftBy(Character->GetActorLocation()+2000);

	Request.Filter.BehaviorDefinitionClasses.Emplace(SmartObjectDefinition);

	TArray<FSmartObjectRequestResult> RequestResults;

	if (SOSubSystem->FindSmartObjects(Request, RequestResults))
	{
		FSmartObjectRequestResult ResultItem = RequestResults.Top();
		FSmartObjectClaimHandle Result = SOSubSystem->Claim(ResultItem);
		UBlackboardComponent* BlackBoardComp = MyController->GetBlackboardComponent();
		if (!IsValid(BlackBoardComp))
		{
			return EBTNodeResult::Failed;
		}
		BlackBoardComp->SetValue<UBlackboardKeyType_SOClaimHandle>(SOClaimedBBSelector.SelectedKeyName, Result);
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;
}