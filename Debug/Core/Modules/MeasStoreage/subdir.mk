################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Modules/MeasStoreage/MS.cpp 

OBJS += \
./Core/Modules/MeasStoreage/MS.o 

CPP_DEPS += \
./Core/Modules/MeasStoreage/MS.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Modules/MeasStoreage/%.o Core/Modules/MeasStoreage/%.su Core/Modules/MeasStoreage/%.cyclo: ../Core/Modules/MeasStoreage/%.cpp Core/Modules/MeasStoreage/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/GPIO" -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/MAX31865" -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/MeasStoreage" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Modules-2f-MeasStoreage

clean-Core-2f-Modules-2f-MeasStoreage:
	-$(RM) ./Core/Modules/MeasStoreage/MS.cyclo ./Core/Modules/MeasStoreage/MS.d ./Core/Modules/MeasStoreage/MS.o ./Core/Modules/MeasStoreage/MS.su

.PHONY: clean-Core-2f-Modules-2f-MeasStoreage

