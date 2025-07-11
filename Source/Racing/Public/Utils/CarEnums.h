// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarEnums.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ECarState : uint8
{
    DRIVING UMETA(DisplayName = "DOWN"),
    FALLING   UMETA(DisplayName = "LEFT"),
    ABILITY      UMETA(DisplayName = "UP")
};