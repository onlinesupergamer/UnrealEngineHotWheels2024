// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WheelCastComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PhysXPublicCore.h"
#include "Car.generated.h"


UCLASS()
class RACING_API ACar : public APawn
{
	GENERATED_BODY()


public:
	// Sets default values for this pawn's properties
	ACar();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float EngineTorque = 3200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SteerTorque = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCurveFloat* EngineCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* CarModel;
	UPROPERTY(EditAnywhere, BlueprintreadOnly)
	class USpringArmComponent* CameraArm;
	UPROPERTY(EditAnywhere, BlueprintreadOnly)
	class UCameraComponent* PlayerCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RayDistance = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Stiffness = 50000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RestLength = 45.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DamperValue = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CounterSteerForce = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FrictionMultiplier = 3.0f;
	float AccelerationValue = 0.0f;
	float SteeringValue = 0.0f;
	float CurrentSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsGrounded;
	FVector GravityDirection;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GravityForce = 1500.0f;
	bool bWasInAirLastFrame = false;
	bool bIsCrashed = false;
	bool bHasExploded = false;
	float CrashTimer = 0.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class UWheelCastComponent*> WheelComponents;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UStaticMeshComponent*> WheelModels;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<USpotLightComponent*> HeadLights;
	TArray<FVector> WheelModelLocations;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void DirectionCheck();
	void SetupWheels();
	void CameraLookUp(float Value);
	void CameraLookRight(float Value);
	void Steer(float Value);
	void Accelerate(float Value);
	void Friction();
	void CounterSteer(float InputValue);
	void UpdateWheelLocations();
	void UpdateWheelRotations();

	UFUNCTION()
	void CollisionHandler(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	void GetCarSpeed();
	void GroundedCheck();
	void HandleGravity();
	void HandleLanding();
	void CarCrash(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	float QLerp(float f1, float f2, float LerpSpeed);
	void DisablePlayerInput();
	void EnablePlayerInput();
	void ExplodeCar();
	void ExplosionCheck();

};



