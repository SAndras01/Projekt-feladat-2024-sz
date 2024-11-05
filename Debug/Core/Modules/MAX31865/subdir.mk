################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Modules/MAX31865/MAX31865.cpp 

OBJS += \
./Core/Modules/MAX31865/MAX31865.o 

CPP_DEPS += \
./Core/Modules/MAX31865/MAX31865.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Modules/MAX31865/%.o Core/Modules/MAX31865/%.su Core/Modules/MAX31865/%.cyclo: ../Core/Modules/MAX31865/%.cpp Core/Modules/MAX31865/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/GPIO" -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/MAX31865" -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/MeasStoreage" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"  -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Modules-2f-MAX31865

clean-Core-2f-Modules-2f-MAX31865:
	-$(RM) ./Core/Modules/MAX31865/MAX31865.cyclo ./Core/Modules/MAX31865/MAX31865.d ./Core/Modules/MAX31865/MAX31865.o ./Core/Modules/MAX31865/MAX31865.su

.PHONY: clean-Core-2f-Modules-2f-MAX31865

