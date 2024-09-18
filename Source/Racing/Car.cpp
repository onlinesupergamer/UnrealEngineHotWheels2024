// Fill out your copyright notice in the Description page of Project Settings.


#include "Car.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"


ACar::ACar()
{

	PrimaryActorTick.bCanEverTick = true;
	WheelComponents.SetNum(4);
	m_Length.SetNum(4);
	m_LastLength.SetNum(4);
	m_Velocity.SetNum(4);
	m_DamperForce.SetNum(4);
	m_Force.SetNum(4);
	m_SuspensionForce.SetNum(4);
	m_Hit.SetNum(4);
	m_bIsGrounded.SetNum(4);
	WheelModels.SetNum(4);
	WheelOffset.SetNum(4);
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
	DirectionCheck();
	GenerateRaycasts(DeltaTime);
	GroundedCheck();
	HandleGravity();
	Friction();
	GetCarSpeed();
	HandleLanding();
	WheelAnimations();

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

	m_Length[0] = 0.0f;
	m_Length[1] = 0.0f;
	m_Length[2] = 0.0f;
	m_Length[3] = 0.0f;

	WheelModels[0] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Front Left Wheel"));
	WheelModels[1] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Front Right Wheel"));
	WheelModels[2] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rear Left Wheel"));
	WheelModels[3] = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rear Right Wheel"));

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
	
	*/

}

void ACar::GenerateRaycasts(float DeltaTime) 
{
	for (int i = 0; i < WheelComponents.Num(); i++) 
	{

		if (WheelComponents.Num() > 0) 
		{
			FVector EndLocation = WheelComponents[i]->GetComponentLocation() + (-WheelComponents[i]->GetUpVector() * RayDistance);


			if (GetWorld()->LineTraceSingleByChannel(m_Hit[i], WheelComponents[i]->GetComponentLocation(), EndLocation, ECC_Visibility))
			{
				if (bIsGrounded)
				{
					bWasPreviouslyInAir = false;
				}

				else 
				{
					bWasPreviouslyInAir = true;

				}

				m_bIsGrounded[i] = true;
				m_LastLength[i] = m_Length[i];
				m_Length[i] = m_Hit[i].Distance;
				m_Velocity[i] = (m_LastLength[i] - m_Length[i]) / DeltaTime;
				m_Force[i] = Stiffness * (RestLength - m_Length[i]);
				m_DamperForce[i] = DamperValue * m_Velocity[i];
				m_SuspensionForce[i] = (m_Force[i] + m_DamperForce[i]) * m_Hit[i].Normal;
				CarModel->AddForceAtLocation(m_SuspensionForce[i], m_Hit[i].Location);
				FVector NewLocation = WheelModels[i]->GetRelativeLocation();
				NewLocation.Z = -m_Length[i] + WheelOffset[i];
				WheelModels[i]->SetRelativeLocation(NewLocation);
				

			}

			else 
			{
				m_bIsGrounded[i] = false;
				FVector NewLocation = WheelModels[i]->GetRelativeLocation();
				NewLocation.Z = -RayDistance + WheelOffset[i];
				WheelModels[i]->SetRelativeLocation(NewLocation);
				

			}

			DrawDebugLine(GetWorld(), WheelComponents[i]->GetComponentLocation(), EndLocation, FColor::Green, false, 0.0f);

		}

	}


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
		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, FString::SanitizeFloat(EngineCurve->GetFloatValue(CurrentSpeed)));

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

void ACar::WheelAnimations() 
{
	int Direction = 1;

	if (FVector::DotProduct(GetVelocity(), GetActorForwardVector())) 
	{
		Direction = 1;
	}

	else
	{	
		if (bIsGrounded) 
		{
			Direction = -1;

		}

	}

	for (int i = 0; i < WheelModels.Num(); i++) 
	{
		if (bIsGrounded) 
		{
			if (i < 2)
			{
				float WheelCirc = 2 * 3.14f * WheelOffset[i];
				float RotAmount = CurrentSpeed / WheelCirc;
				WheelModels[i]->AddLocalRotation(FQuat(FRotator(-RotAmount * Direction, 0, 0)));
			}

			if (i > 1)
			{
				if (AccelerationValue != 0)
				{
					WheelModels[i]->AddLocalRotation(FQuat(FRotator(-AccelerationValue * 10.0f, 0, 0)));
				}

				else
				{
					float WheelCirc = 2 * 3.14f * WheelOffset[i];
					float RotAmount = CurrentSpeed / WheelCirc;
					WheelModels[i]->AddLocalRotation(FQuat(FRotator(-RotAmount * Direction, 0, 0)));
				}
			}
		}

		else 
		{
			if (i > 1) 
			{
				if (AccelerationValue != 0) 
				{
					WheelModels[i]->AddLocalRotation(FQuat(FRotator(-AccelerationValue * 10.0f, 0, 0)));

				}

				else 
				{
					WheelModels[i]->AddLocalRotation(FQuat(FRotator(-3.0f * Direction, 0, 0)));

				}
			}

			if (i < 2) 
			{
				WheelModels[i]->AddLocalRotation(FQuat(FRotator(-3.0f * Direction, 0, 0)));

			}

		}
	}

	WheelComponents[0]->SetRelativeRotation(FQuat(FRotator(0, 25 * SteeringValue, 0)));
	WheelComponents[1]->SetRelativeRotation(FQuat(FRotator(0, 25 * SteeringValue, 0)));

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
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, TEXT("High Speed Impact From Front While Grounded"));
				float UpwardForce = FVector::DotProduct(GetActorUpVector(), GetVelocity());
				float ForwardImpactForce = FVector::DotProduct(GetActorForwardVector(), GetVelocity());
				float UpwardRotationalForce = FVector::DotProduct(GetActorRightVector(), CarModel->GetPhysicsAngularVelocityInRadians());
				CarModel->SetPhysicsAngularVelocityInRadians(FVector(0,0,0));
				CarModel->SetPhysicsLinearVelocity(FVector(0,0,0));
				CarModel->SetPhysicsAngularVelocityInRadians(-CarModel->GetRightVector() * 5.0f);
				//Reduce Collision Impulse;

			}
		}

		if (!bIsGrounded) 
		{
			FVector PrevRotVelocity = CarModel->GetPhysicsAngularVelocityInRadians();
		}


		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::FromInt(TDeg));

		
	}

}

void ACar::HandleLanding() 
{
	if (bWasPreviouslyInAir) 
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
	if (m_bIsGrounded[0] || m_bIsGrounded[1] || m_bIsGrounded[2] || m_bIsGrounded[3]) 
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





