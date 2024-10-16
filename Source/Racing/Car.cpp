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
		CarModel->AddForce(GetActorForwardVector() * ((EngineTorque * EngineCurve->GetFloatValue(CurrentSpeed)) * Value), TEXT("None"), true);
	}
	AccelerationValue = Value;
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
			WheelModelLocations[i].Z = -WheelComponents[i]->m_Length + WheelComponents[i]->WheelsRadius;
		}
		else 
		{
			WheelModelLocations[i].Z = -WheelComponents[i]->RayDistance + WheelComponents[i]->WheelsRadius;
		}
		WheelModels[i]->SetRelativeLocation(WheelModelLocations[i]);
	}
}

void ACar::UpdateWheelRotations() 
{
	for (int i = 0; i < WheelComponents.Num(); i++) 
	{
		FQuat WheelRotation;
		float WheelCirc = 2 * 3.14 * WheelComponents[i]->WheelsRadius;
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
			WheelComponents[i]->SetRelativeRotation(FRotator(0, 25 * SteeringValue, 0));
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
	
	if (bHasImpactThisFrame) 
	{
		return;
	}
	bHasImpactThisFrame = true;
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

			if (GetVelocity().Size() > 750.0f) 
			{
				CarCrash(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Crashed In Air"));

			}
		}
	}

	bHasImpactThisFrame = false;
}

void ACar::CollisionRelease() 
{

}

void ACar::CarCrash(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsCrashed) 
	{
		return;
	}

	UWheelCastComponent* ClosesWheel = nullptr;
	UStaticMeshComponent* WheelMesh = nullptr;
	float ClosestDistance = 500.0f;

	for (int i = 0; i < WheelComponents.Num(); i++) 
	{
		float _Distance = FVector::Dist(WheelComponents[i]->GetComponentLocation(), Hit.Location);

		if (_Distance < ClosestDistance) 
		{
			ClosestDistance = _Distance;
			ClosesWheel = WheelComponents[i];
			WheelMesh = WheelModels[i];
		}

	}

	if (ClosesWheel != nullptr && ClosesWheel->bIsWheelActive) 
	{
		WheelMesh->SetVisibility(false);
		ClosesWheel->bIsWheelActive = false;
	}

	bIsCrashed = true;

}

void ACar::ExplosionCheck() 
{
	if (bIsCrashed)
	{
		CrashTimer += 0.01f;

		if (GetVelocity().Size() < 400.0f && !bHasExploded || CrashTimer >= 2.5f)
		{
			ExplodeCar();
			bHasExploded = true;
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::FromInt(CrashTimer));

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
	if (bIsGrounded && CurrentSpeed >= 650) 
	{
		GravityDirection = -GetActorUpVector();
	}
	else 
	{
		GravityDirection = FVector(0, 0, -1);
	}

	if (bIsCrashed) 
	{
		GravityDirection = FVector(0, 0, -1);

	}

	CarModel->AddForce(GravityDirection * GravityForce, TEXT("None"), true);
}

void ACar::ExplodeCar()
{
	if (bHasExploded) 
	{
		return;
	}
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Explode"));

	//this->Destroy();

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





