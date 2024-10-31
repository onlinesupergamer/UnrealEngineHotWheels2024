// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Car.h"
#include "WheelCastComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RACING_API UWheelCastComponent : public USceneComponent
{
	GENERATED_BODY()

	FVector WheelLocation;

public:	
	// Sets default values for this component's properties
	UWheelCastComponent();
	class ACar* Car = Cast<class ACar>(GetOwner());
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RayDistance;
	float m_Length;
	float m_LastLength;
	float m_Velocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float m_Stiffness;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float m_RestLength;
	float m_DamperForce;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float m_DamperValue;
	float m_Force;
	FVector m_SuspensionForce;
	FHitResult m_Hit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WheelsRadius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HorizontalOffset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWheelIsGrounded;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSteer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDriveWheel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsWheelActive = true;

	bool bWasInAirThisFrame;


	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void GenerateRaycasts(float DeltaTime);
	void GenerateSweepCasts(float DeltaTime);

	


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



};


