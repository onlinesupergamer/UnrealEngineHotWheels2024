// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WheelCastComponent.h"
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
	float EngineTorque = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SteerTorque = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UCurveFloat* EngineCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* CarModel;
	UPROPERTY(EditAnywhere, BlueprintreadOnly)
	class USpringArmComponent* CameraArm;
	UPROPERTY(EditAnywhere, BlueprintreadOnly)
	class UCameraComponent* PlayerCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RayDistance = 35.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Stiffness = 10000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RestLength = 40.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DamperValue = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CounterSteerForce = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FrictionMultiplier = 5.0f;

	float CurrentSpeed;
	bool bIsGrounded;
	FVector GravityDirection;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GravityForce = 100.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<class UWheelCastComponent*> WheelComponents;

	


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void DirectionCheck();
	void SetupWheels();
	void GenerateRaycasts(float DeltaTime);
	void CameraLookUp(float Value);
	void CameraLookRight(float Value);
	void Steer(float Value);
	void Accelerate(float Value);
	void Friction();
	void CounterSteer(float InputValue);

	UFUNCTION()
	void CollisionHandler(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	void GetCarSpeed();
	void GroundedCheck();
	void HandleGravity();
	
	TArray<float> m_Length;
	TArray<float> m_LastLength;
	TArray<float> m_Velocity;
	TArray<float> m_DamperForce;
	TArray<float> m_Force;
	TArray<bool> m_bIsGrounded;
	TArray<FVector> m_SuspensionForce;
	TArray<FHitResult> m_Hit;

};



