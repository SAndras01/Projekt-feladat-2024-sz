################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Modules/GPIO/GPIO.cpp 

OBJS += \
./Core/Modules/GPIO/GPIO.o 

CPP_DEPS += \
./Core/Modules/GPIO/GPIO.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Modules/GPIO/%.o Core/Modules/GPIO/%.su Core/Modules/GPIO/%.cyclo: ../Core/Modules/GPIO/%.cpp Core/Modules/GPIO/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/GPIO" -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/MAX31865" -I"D:/Projektek/ST32Projektek/ProjektFeladat/Core/Modules/MeasStoreage" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Modules-2f-GPIO

clean-Core-2f-Modules-2f-GPIO:
	-$(RM) ./Core/Modules/GPIO/GPIO.cyclo ./Core/Modules/GPIO/GPIO.d ./Core/Modules/GPIO/GPIO.o ./Core/Modules/GPIO/GPIO.su

.PHONY: clean-Core-2f-Modules-2f-GPIO

