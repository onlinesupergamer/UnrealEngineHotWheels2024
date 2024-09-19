// Fill out your copyright notice in the Description page of Project Settings.


#include "Car.h"
#include "DrawDebugHelpers.h"
#include "WheelCastComponent.h"
#include "Kismet/KismetMathLibrary.h"


ACar::ACar()
{

	PrimaryActorTick.bCanEverTick = true;
	WheelComponents.SetNum(4);
	WheelModels.SetNum(4);
	HeadLights.SetNum(2);
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
	UpdateWheels();

	
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
	AddControllerPitchInput((Value * 45.0f) * GetWorld()->DeltaTimeSeconds);
}

void ACar::CameraLookRight(float Value) 
{
	AddControllerYawInput((Value * 45.0f) * GetWorld()->DeltaTimeSeconds);
}

void ACar::Accelerate(float Value) 
{
	if (bIsGrounded) 
	{
		if (EngineCurve == nullptr) 
		{
			return;
		}
		CarModel->AddForce(GetActorForwardVector() * ((EngineTorque * EngineCurve->GetFloatValue(CurrentSpeed)) * Value), TEXT("None"), true);
	}
	AccelerationValue = Value;
}

void ACar::Steer(float Value) 
{
	if (bIsGrounded) 
	{
		CarModel->AddTorqueInRadians(GetActorUpVector() * (SteerTorque * Value), TEXT("None"), true);
		CounterSteer(Value);
	}
	SteeringValue = Value;
}

void ACar::UpdateWheels() 
{
	for (int i = 0; i < WheelComponents.Num(); i++) 
	{
		FVector WheelLocation = WheelModels[i]->GetRelativeLocation();
		WheelLocation.Y = WheelComponents[i]->HorizontalOffset;
		if (WheelComponents[i]->bWheelIsGrounded) 
		{
			WheelLocation.Z = -WheelComponents[i]->m_Length + WheelComponents[i]->WheelsRadius;
		}
		else 
		{
			WheelLocation.Z = -WheelComponents[i]->RayDistance + WheelComponents[i]->WheelsRadius;
		}
		WheelModels[i]->SetRelativeLocation(WheelLocation);
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
	if (!OtherActor->ActorHasTag(TEXT("Track"))) 
	{
		float P = FVector::DotProduct(Hit.ImpactNormal, GetActorForwardVector());
		float T = FMath::Acos(P);
		float TDeg = FMath::RadiansToDegrees(T);
		
		if (bIsGrounded) 
		{
			if (TDeg >= 120 && CurrentSpeed >= 1500)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("High Speed Impact From Front While Grounded"));
				float UpwardForce = FVector::DotProduct(GetActorUpVector(), GetVelocity());
				float ForwardImpactForce = FVector::DotProduct(GetActorForwardVector(), GetVelocity());
				float UpwardRotationalForce = FVector::DotProduct(GetActorRightVector(), CarModel->GetPhysicsAngularVelocityInRadians());
				float XRotVel = CarModel->GetPhysicsAngularVelocityInRadians().X;
				float YRotVel = CarModel->GetPhysicsAngularVelocityInRadians().Y;
				float ZRotVel = CarModel->GetPhysicsAngularVelocityInRadians().Z;
				float XVel = CarModel->GetPhysicsLinearVelocity().X;
				float YVel = CarModel->GetPhysicsLinearVelocity().Y;
				float ZVel = CarModel->GetPhysicsLinearVelocity().Z;
				CarModel->SetPhysicsAngularVelocityInRadians(FVector(XRotVel,YRotVel,0));
				CarModel->SetPhysicsLinearVelocity(FVector(0,YVel,0));
				//Reduce Collision Impulse;
				CarCrash(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
			}
		}

		if (!bIsGrounded) 
		{
			FVector PrevRotVelocity = CarModel->GetPhysicsAngularVelocityInRadians();
		}
	}
}

void ACar::CarCrash(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	int idx = FMath::RandRange(0, 3);
	WheelModels[idx]->SetVisibility(false);
	
	DisablePlayerInput();
}

void ACar::DisablePlayerInput() 
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	ACar::DisableInput(PlayerController);
}

void ACar::EnablePlayerInput() 
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	ACar::EnableInput(PlayerController);
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
	m_CurrentSpeed = FMath::Abs(FMath::TruncToInt(m_CurrentSpeed));
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::SanitizeFloat(m_CurrentSpeed));
	CurrentSpeed = m_CurrentSpeed;

}
void ACar::GroundedCheck() 
{
	if (!WheelComponents.IsValidIndex(0)) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("None Found"));

		return;
	}
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("Found"));


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
	if (bIsGrounded && CurrentSpeed >= 650) 
	{
		GravityDirection = -GetActorUpVector();
	}
	else 
	{
		GravityDirection = FVector(0, 0, -1);
	}

	CarModel->AddForce(GravityDirection * GravityForce, TEXT("None"), true);
}

void ACar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	PlayerInputComponent->BindAxis("LookUp", this, &ACar::CameraLookUp);
	PlayerInputComponent->BindAxis("LookRight", this, &ACar::CameraLookRight);
	PlayerInputComponent->BindAxis("Accelerate", this, &ACar::Accelerate);
	PlayerInputComponent->BindAxis("Steer", this, &ACar::Steer);
}

float ACar::QLerp(float f1, float f2, float LerpSpeed) 
{
	float f = FMath::Lerp(f1, f2, (1.0f / LerpSpeed) * GetWorld()->DeltaTimeSeconds);
	return f;
}





