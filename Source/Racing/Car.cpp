// Fill out your copyright notice in the Description page of Project Settings.


#include "Car.h"
#include "DrawDebugHelpers.h"
#include "WheelCastComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysXPublic.h"


ACar::ACar()
{

	PrimaryActorTick.bCanEverTick = true;
	WheelComponents.SetNum(4);
	WheelModels.SetNum(4);
	HeadLights.SetNum(2);
	WheelModelLocations.SetNum(4);
	//Headlight amount could be a variable for each car

	CarModel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Car Mesh"));
	CarModel->SetupAttachment(RootComponent);
	CarModel->SetSimulatePhysics(true);

	SetupWheels();

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Arm"));
	CameraArm->SetupAttachment(CarModel);
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	PlayerCamera->SetupAttachment(CameraArm);
	
	CameraArm->bUsePawnControlRotation = true;
	CameraArm->SocketOffset = FVector(0, 0, 75);
	CameraArm->TargetArmLength = 350.0f;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagMaxDistance = 100.0f;
	CarModel->SetNotifyRigidBodyCollision(true);

}

void ACar::BeginPlay()
{
	Super::BeginPlay();
	CarModel->OnComponentHit.AddDynamic(this, &ACar::CollisionHandler);
	
}


void ACar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	GroundedCheck();
	HandleGravity();
	Friction();
	GetCarSpeed();
	HandleLanding();
	UpdateWheelLocations();
	UpdateWheelRotations();
	ExplosionCheck();
	CameraHandler();

	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::FromInt(CarModel->GetPhysicsAngularVelocity().Z));

}

void ACar::DirectionCheck()
{
	DrawDebugLine(GetWorld(), CarModel->GetComponentLocation(), CarModel->GetForwardVector() * 225.0f + CarModel->GetComponentLocation(), FColor::Purple, false, 0.0f);
	DrawDebugLine(GetWorld(), CarModel->GetComponentLocation(), CarModel->GetRightVector() * 125.0f + CarModel->GetComponentLocation(), FColor::Red, false, 0.0f);

}

void ACar::SetupWheels() 
{
	WheelComponents[0] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Front Left"));
	WheelComponents[1] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Front Right"));
	WheelComponents[2] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Rear Left"));
	WheelComponents[3] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Rear Right"));

	WheelComponents[0]->SetupAttachment(CarModel);
	WheelComponents[1]->SetupAttachment(CarModel);
	WheelComponents[2]->SetupAttachment(CarModel);
	WheelComponents[3]->SetupAttachment(CarModel);

	WheelModels[0] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Front Left Wheel Model"));
	WheelModels[1] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Front Right Wheel Model"));
	WheelModels[2] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rear Left Wheel Model"));
	WheelModels[3] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rear Right Wheel Model"));

	WheelModels[0]->SetupAttachment(WheelComponents[0]);
	WheelModels[1]->SetupAttachment(WheelComponents[1]);
	WheelModels[2]->SetupAttachment(WheelComponents[2]);
	WheelModels[3]->SetupAttachment(WheelComponents[3]);

	HeadLights[0] = CreateDefaultSubobject<USpotLightComponent>("HeadLight Left");
	HeadLights[1] = CreateDefaultSubobject<USpotLightComponent>("HeadLight Right");

	HeadLights[0]->SetupAttachment(CarModel);
	HeadLights[1]->SetupAttachment(CarModel);

	/*
		This function could be optimized as a forloop
		Maybe
	*/

}


void ACar::CameraLookUp(float Value) 
{
	return;

	if (Value >= InputDeadZone || Value <= -InputDeadZone)
	{
		AddControllerPitchInput((Value * 45.0f) * GetWorld()->DeltaTimeSeconds);

	}
}

void ACar::CameraLookRight(float Value) 
{
	return;

	if (Value >= InputDeadZone || Value <= - InputDeadZone) 
	{
		AddControllerYawInput((Value * 45.0f) * GetWorld()->DeltaTimeSeconds);

	}
}

void ACar::Accelerate(float Value) 
{
	if (bIsCrashed)
	{
		AccelerationValue = 0;
		return;
	}

	if (bIsGrounded) 
	{
		if (EngineCurve == nullptr) 
		{
			return;
		}
		FVector DriveNormal = GroundNormal;
		FVector ProjectedNormal = FVector::VectorPlaneProject(GetActorForwardVector(), GroundNormal);

		CarModel->AddForce(ProjectedNormal * ((EngineTorque * EngineCurve->GetFloatValue(CurrentSpeed)) * Value), TEXT("None"), true);
	}
	AccelerationValue = Value;
}

void ACar::CameraHandler() 
{
	if (bIsGrounded) 
	{
		CameraArm->bInheritPitch = true;
		CameraArm->bInheritRoll = true;
	}

	else
	{
		CameraArm->bInheritPitch = false;
		CameraArm->bInheritRoll = false;
	}
}

void ACar::Steer(float Value) 
{
	if (bIsCrashed) 
	{
		SteeringValue = 0;
		return;
	}

	if (bIsGrounded) 
	{
		CarModel->AddTorqueInRadians(GetActorUpVector() * (SteerTorque * Value), TEXT("None"), true);
		CounterSteer(Value);
	}
	SteeringValue = Value;
}

void ACar::UpdateWheelLocations() 
{
	for (int i = 0; i < WheelComponents.Num(); i++) 
	{
		WheelModelLocations[i] = WheelModels[i]->GetRelativeLocation();
		WheelModelLocations[i].Y = WheelComponents[i]->HorizontalOffset;

		if (WheelComponents[i]->bWheelIsGrounded) 
		{
			WheelModelLocations[i].Z = -WheelComponents[i]->m_Length - WheelComponents[i]->WheelRadius;
		}
		else 
		{
			
		}
		
		WheelModels[i]->SetRelativeLocation(WheelModelLocations[i]);
	}
}

void ACar::UpdateWheelRotations() 
{
	for (int i = 0; i < WheelComponents.Num(); i++) 
	{
		FQuat WheelRotation;
		float WheelCirc = 2 * 3.14 * WheelComponents[i]->WheelRadius;
		float RotationAmount = CurrentSpeed / WheelCirc;

		if (WheelComponents[i]->bWheelIsGrounded) 
		{
			if (WheelComponents[i]->bIsDriveWheel)
			{
				if (AccelerationValue != 0) 
				{
					WheelRotation = FQuat(FRotator(-AccelerationValue * 10.0f, 0, 0));
					WheelModels[i]->AddLocalRotation(WheelRotation);
				}
				else 
				{
					WheelRotation = FQuat(FRotator(-RotationAmount, 0, 0));
					WheelModels[i]->AddLocalRotation(WheelRotation);
				}
			}
			if (WheelComponents[i]->bIsSteer) 
			{
				WheelRotation = FQuat(FRotator(-RotationAmount, 0, 0));
				WheelModels[i]->AddLocalRotation(WheelRotation);
			}
		}

		else 
		{
			if (WheelComponents[i]->bIsDriveWheel)
			{
				if (AccelerationValue != 0)
				{
					WheelRotation = FQuat(FRotator(-AccelerationValue * 10.0f, 0, 0));
					WheelModels[i]->AddLocalRotation(WheelRotation);
				}
				else
				{
					WheelRotation = FQuat(FRotator(-2, 0, 0));
					WheelModels[i]->AddLocalRotation(WheelRotation);
				}
			}
			if (WheelComponents[i]->bIsSteer)
			{
				WheelRotation = FQuat(FRotator(-2, 0, 0));
				WheelModels[i]->AddLocalRotation(WheelRotation);
			}
		}

		if (WheelComponents[i]->bIsSteer) 
		{
			WheelComponents[i]->SetRelativeRotation(FRotator(0, 35 * SteeringValue, 0));
		}
	}

}

void ACar::Friction() 
{
	if (bIsGrounded) 
	{
		float SidewaysSpeed = FVector::DotProduct(GetVelocity(), GetActorRightVector());
		CarModel->AddForce((-GetActorRightVector() * SidewaysSpeed) * FrictionMultiplier, TEXT("None"), true);
	}
}

void ACar::CounterSteer(float InputValue) 
{
	if (bIsCrashed)
	{
		return;
	}

	FVector RotationalVelocity = CarModel->GetPhysicsAngularVelocityInRadians();
	float RotationSpeed = FVector::DotProduct(RotationalVelocity, GetActorUpVector());
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::SanitizeFloat(RotationSpeed));
	if (InputValue == 0) 
	{
		CarModel->AddTorqueInRadians((-GetActorUpVector() * RotationSpeed) * 5.0f, TEXT("None"), true);
		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, TEXT("No Steering Input"));
	}

}

void ACar::CollisionHandler(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{


}

void ACar::ExplosionCheck() 
{
	if (bIsCrashed)
	{
		CrashTimer += 0.01f;

		if (GetVelocity().Size() < 400.0f && !bHasExploded || CrashTimer >= 2.5f)
		{
			bHasExploded = true;
		}
	}

	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::FromInt(CrashTimer));

}

void ACar::DisablePlayerInput() 
{
	//APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	//ACar::DisableInput(PlayerController);
}

void ACar::EnablePlayerInput() 
{
	//APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	//ACar::EnableInput(PlayerController);
}

void ACar::HandleLanding() 
{
	if (bWasInAirLastFrame) 
	{
		
	}
}

void ACar::GetCarSpeed() 
{
	float m_CurrentSpeed = FVector::DotProduct(GetVelocity(), GetActorForwardVector());
	m_CurrentSpeed = FMath::TruncToInt(m_CurrentSpeed);
	CurrentSpeed = m_CurrentSpeed;

}
void ACar::GroundedCheck() 
{
	if (!WheelComponents.IsValidIndex(0)) 
	{
		return;
	}

	if (WheelComponents[0]->bWheelIsGrounded || WheelComponents[1]->bWheelIsGrounded || WheelComponents[2]->bWheelIsGrounded || WheelComponents[3]->bWheelIsGrounded)
	{
		bIsGrounded = true;
	}

	else 
	{
		bIsGrounded = false;
	}
}

void ACar::HandleGravity() 
{
	if (bIsGrounded) 
	{
		CarModel->AddForce(-CarModel->GetUpVector() * 500.0f, TEXT("None"), true);
	}

	else 
	{
		CarModel->AddForce(FVector(0,0,-1) * 2500.0f, TEXT("None"), true);
	}
}

void ACar::DebugWheels()
{
	bool bVisible = true;

	for (int i = 0; i < WheelModels.Num(); i++) 
	{
		if (bVisible) 
		{
			WheelModels[i]->SetVisibility(false);
			bVisible = false;
		}
		else 
		{
			WheelModels[i]->SetVisibility(true);
			bVisible = true;
		}
	}
}

void ACar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	PlayerInputComponent->BindAxis("LookUp", this, &ACar::CameraLookUp);
	PlayerInputComponent->BindAxis("LookRight", this, &ACar::CameraLookRight);
	PlayerInputComponent->BindAxis("Accelerate", this, &ACar::Accelerate);
	PlayerInputComponent->BindAxis("Steer", this, &ACar::Steer);
	PlayerInputComponent->BindAction("DebugWheel", IE_Pressed, this, &ACar::DebugWheels);
}

float ACar::QLerp(float f1, float f2, float LerpSpeed) 
{
	float f = FMath::Lerp(f1, f2, (1.0f / LerpSpeed) * GetWorld()->DeltaTimeSeconds);
	return f;
}





