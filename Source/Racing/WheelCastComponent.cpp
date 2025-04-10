// Fill out your copyright notice in the Description page of Project Settings.


#include "WheelCastComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UWheelCastComponent::UWheelCastComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
}

// Called when the game starts
void UWheelCastComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UWheelCastComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	GenerateRaycasts(DeltaTime);
	//GenerateSweepCasts(DeltaTime);
}

void UWheelCastComponent::GenerateRaycasts(float DeltaTime) 
{

	FVector EndLocation = GetComponentLocation() + (-GetUpVector() * RayDistance);

	if (GetWorld()->LineTraceSingleByChannel(m_Hit, GetComponentLocation(), EndLocation, ECC_Visibility)) 
	{
		
		m_LastLength = m_Length;
		m_Length = m_Hit.Distance;
		m_Velocity = (m_LastLength - m_Length) / DeltaTime;
		m_Force = m_Stiffness * (m_RestLength - m_Length);
		m_DamperForce = m_DamperValue * m_Velocity;
		m_SuspensionForce = (m_Force + m_DamperForce) * GetUpVector();
		
		Car->GroundNormal = m_Hit.Normal;
		Car->CarModel->AddForceAtLocation(m_SuspensionForce, m_Hit.Location);
		bWheelIsGrounded = true;

	}

	else
	{
		bWheelIsGrounded = false;
	}

	
}

void UWheelCastComponent::GenerateSweepCasts(float DeltaTime)
{
	if (!bIsWheelActive)
	{
		return;
	}

	FVector EndLocation = GetComponentLocation() + (-GetUpVector() * RayDistance);

	if (GetWorld()->LineTraceSingleByChannel(m_Hit, GetComponentLocation(), EndLocation, ECC_Visibility))
	{

		

	}

	else
	{
		

	}
}


