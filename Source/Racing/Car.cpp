// Fill out your copyright notice in the Description page of Project Settings.


/*

	If Car takes too much damage within a specific timeframe, then explode
	Using a health system that resets back to full health after a second or two
	Only explodes if the damage sources take all health within that timeframe

	Use Mouse to control car rotation when using the Jump Jets


*/



#include "Car.h"
#include "DrawDebugHelpers.h"
#include "WheelCastComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"




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
	JumpJets();
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::FromInt(CarModel->GetPhysicsAngularVelocity().Z));
	
	if (CarState == ECarState::ABILITY) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, TEXT("Enum Is On Ability!"));

	}

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

	WheelModels[0]->SetRelativeLocation(WheelModelLocations[0]);
	WheelModels[1]->SetRelativeLocation(WheelModelLocations[1]);
	WheelModels[2]->SetRelativeLocation(WheelModelLocations[2]);
	WheelModels[3]->SetRelativeLocation(WheelModelLocations[3]);

	/*
		This function could be optimized as a forloop
		Maybe
	*/

}


void ACar::CameraLookUp(float Value) 
{
	if (Value >= InputDeadZone || Value <= -InputDeadZone)
	{
		AddControllerPitchInput((Value * 45.0f) * GetWorld()->DeltaTimeSeconds);

	}
}

void ACar::CameraLookRight(float Value) 
{
	if (Value >= InputDeadZone || Value <= - InputDeadZone) 
	{
		AddControllerYawInput((Value * 45.0f) * GetWorld()->DeltaTimeSeconds);

	}
}

void ACar::Accelerate(float Value) 
{
	if (bIsGrounded) 
	{
		if (EngineCurve == ((void*)0))
		{
			return;
		}

		FVector DriveNormal = GroundNormal;
		FVector ProjectedNormal = FVector::VectorPlaneProject(GetActorForwardVector(), GroundNormal);

		CarModel->AddForce(ProjectedNormal * ((EngineTorque * EngineCurve->GetFloatValue(CurrentSpeed)) * Value), TEXT("None"), true);
	}
	AccelerationValue = Value;

	if (bJetsActive) 
	{
		CarModel->AddForce((CarModel->GetForwardVector() * (Value * 500)), TEXT("None"), true);

	}
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
	if (bIsGrounded) 
	{
		CarModel->AddTorqueInRadians(GetActorUpVector() * (SteerTorque * Value), TEXT("None"), true);
		CounterSteer(Value);
	}

	if (bJetsActive) 
	{
		CarModel->AddTorqueInRadians(GetActorUpVector() * (5 * Value), TEXT("None"), true);
		CounterSteer(Value);

	}

	SteeringValue = Value;
}

void ACar::UpdateWheelLocations() 
{
	for (int i = 0; i < WheelComponents.Num(); i++) 
	{	
		if (WheelComponents[i]->bWheelIsGrounded) 
		{
			WheelModelLocations[i].Z = -WheelComponents[i]->m_Hit.Distance + WheelComponents[i]->WheelRadius;
			WheelModels[i]->SetRelativeLocation(WheelModelLocations[i]);
		}

		else 
		{
			WheelModelLocations[i].Z = QLerp(WheelModelLocations[i].Z, -WheelComponents[i]->RayDistance, 0.1f);
			WheelModels[i]->SetRelativeLocation(WheelModelLocations[i]);
		}
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
	FVector RotationalVelocity = CarModel->GetPhysicsAngularVelocityInRadians();
	float RotationSpeed = FVector::DotProduct(RotationalVelocity, GetActorUpVector());
	if (InputValue == 0) 
	{
		CarModel->AddTorqueInRadians((-GetActorUpVector() * RotationSpeed) * 5.0f, TEXT("None"), true);
	
	}

}

void ACar::CollisionHandler(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, TEXT("Hit"));
	FVector HitNormal = Hit.Normal;
	float dot = FVector::DotProduct(NormalImpulse, CarModel->GetUpVector());


	if (dot >= 0) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, TEXT("Hit From Bottom"));
		FVector AngVel = HitComp->GetPhysicsAngularVelocityInRadians();
	

		//HitComp->AddAngularImpulseInRadians(-AngVel);
	}


}

void ACar::ExplosionCheck() 
{


}

void ACar::DisablePlayerInput() 
{

}

void ACar::EnablePlayerInput() 
{

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

	if (WheelComponents[0]->bWheelIsGrounded && WheelComponents[1]->bWheelIsGrounded && WheelComponents[2]->bWheelIsGrounded && WheelComponents[3]->bWheelIsGrounded)
	{
		bIsGrounded = true;
		CarState = ECarState::DRIVING;
	}

	else 
	{
		bIsGrounded = false;
		CarState = ECarState::FALLING;

	}

	
}

void ACar::HandleGravity() 
{
	if (CarState == ECarState::ABILITY) 
	{
		return;
	}

	if (bIsGrounded) 
	{
		CarModel->AddForce(-CarModel->GetUpVector() * 100.0f, TEXT("None"), true);
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

void ACar::ActivateAbility()
{
	if (bHasJets) 
	{
		//The jets should line the car with world up slowly

		if (!bJetsActive) 
		{
			//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, TEXT("Jump"));
			CarModel->SetPhysicsAngularVelocity(FVector(0, 0, CarModel->GetPhysicsAngularVelocity().Z));
			//CarModel->AddImpulse(CarModel->GetUpVector() * 850.0f, TEXT("None"), true);
			bJetsActive = true;
		}
		else 
		{
			bJetsActive = false;

		}

	}
	
}

void ACar::JumpJets()
{
	if (bJetsActive) 
	{
		FHitResult m_Hit;
		FVector EndLocation = GetActorLocation() + (-GetActorUpVector() * 160.0f);

		CarState = ECarState::ABILITY;

		if (GetWorld()->LineTraceSingleByChannel(m_Hit, GetActorLocation(), EndLocation, ECC_Visibility)) 
		{
			FVector HitLocation;
			HitLocation = m_Hit.ImpactPoint;
			
			FVector HoverLocation = HitLocation + (m_Hit.Normal * 150.0f);
			//SetActorLocation(HoverLocation);

			CarModel->AddImpulse(m_Hit.Normal * 100.0f, TEXT("None"), true);

		}

		else 
		{
			/*
				 Force Car to flip using World Up		
			
			
			*/
			FVector CarRotation = GetActorRotation().Vector();


		}

		DrawDebugLine(GetWorld(), GetActorLocation(), EndLocation, FColor::Purple, false, 0.0f);
	}

}

void ACar::JetTiltForward(float Value)
{
	if (bJetsActive) 
	{
		//CarModel->AddRelativeRotation(FQuat(1,0,0,0));
	}
}


void ACar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	PlayerInputComponent->BindAxis("LookUp", this, &ACar::CameraLookUp);
	PlayerInputComponent->BindAxis("LookRight", this, &ACar::CameraLookRight);
	PlayerInputComponent->BindAxis("Accelerate", this, &ACar::Accelerate);
	PlayerInputComponent->BindAxis("Steer", this, &ACar::Steer);
	PlayerInputComponent->BindAction("DebugWheel", IE_Pressed, this, &ACar::DebugWheels);
	PlayerInputComponent->BindAction("Ability", IE_Pressed, this, &ACar::ActivateAbility);
	PlayerInputComponent->BindAction("Ability", IE_Released, this, &ACar::ActivateAbility);
	PlayerInputComponent->BindAxis("JetsTiltForward", this, &ACar::JetTiltForward);
	//////////GRAPPLE AIM
}

float ACar::QLerp(float f1, float f2, float LerpSpeed) 
{
	float f = FMath::Lerp(f1, f2, (1.0f / LerpSpeed) * GetWorld()->DeltaTimeSeconds);
	return f;
}





