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
	HandleGravity();
	GroundedCheck();
	Friction();
	GetCarSpeed();


}

void ACar::DirectionCheck()
{
	DrawDebugLine(GetWorld(), CarModel->GetComponentLocation(), CarModel->GetForwardVector() * 225.0f + CarModel->GetComponentLocation(), FColor::Purple, false, 0.0f);
	DrawDebugLine(GetWorld(), CarModel->GetComponentLocation(), CarModel->GetRightVector() * 125.0f + CarModel->GetComponentLocation(), FColor::Red, false, 0.0f);

}

void ACar::SetupWheels() 
{
	WheelComponents[0] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Front Left"));
	WheelComponents[0]->SetupAttachment(CarModel);
	WheelComponents[1] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Front Right"));
	WheelComponents[1]->SetupAttachment(CarModel);
	WheelComponents[2] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Rear Left"));
	WheelComponents[2]->SetupAttachment(CarModel);
	WheelComponents[3] = CreateDefaultSubobject<UWheelCastComponent>(TEXT("Rear Right"));
	WheelComponents[3]->SetupAttachment(CarModel);

	m_Length[0] = 0.0f;
	m_Length[1] = 0.0f;
	m_Length[2] = 0.0f;
	m_Length[3] = 0.0f;


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
				m_LastLength[i] = m_Length[i];
				m_Length[i] = m_Hit[i].Distance;
				m_Velocity[i] = (m_LastLength[i] - m_Length[i]) / DeltaTime;
				m_Force[i] = Stiffness * (RestLength - m_Length[i]);
				m_DamperForce[i] = DamperValue * m_Velocity[i];
				m_SuspensionForce[i] = (m_Force[i] + m_DamperForce[i]) * m_Hit[i].Normal;
				CarModel->AddForceAtLocation(m_SuspensionForce[i], m_Hit[i].Location);
				m_bIsGrounded[i] = true;

			}

			else 
			{
				m_bIsGrounded[i] = false;


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

		CarModel->AddForce(GetActorForwardVector() * ((EngineTorque * EngineCurve->GetFloatValue(CurrentSpeed)) * Value), TEXT("None"), true);
		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, FString::SanitizeFloat(EngineCurve->GetFloatValue(CurrentSpeed)));

	}


}

void ACar::Steer(float Value) 
{
	if (bIsGrounded) 
	{
		CarModel->AddTorqueInRadians(GetActorUpVector() * (SteerTorque * Value), TEXT("None"), true);
		CounterSteer(Value);
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
	FVector NewRotVelocity = CarModel->GetPhysicsAngularVelocityInRadians();
	NewRotVelocity.X = 0.0f;
	NewRotVelocity.Y = 0.0f;
	//NewRotVelocity.Z = 0.0f;
	CarModel->SetPhysicsAngularVelocityInRadians(NewRotVelocity);
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
	if (bIsGrounded && CurrentSpeed >= 650.0f) 
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





