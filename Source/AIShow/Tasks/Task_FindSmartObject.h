// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "CoreMinimal.h"
#include "Task_FindSmartObject.generated.h"

class USmartObjectBehaviorDefinition;
class UGameplayBehavior;
class USmartObjectComponent;

UCLASS()
class AISHOW_API UTask_FindSmartObject : public UBTTaskNode
{
	GENERATED_BODY()
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent &OwnerComp, uint8 *NodeMemory) override;
public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<USmartObjectBehaviorDefinition> SmartObjectDefinition;
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector SOClaimedBBSelector;
};
