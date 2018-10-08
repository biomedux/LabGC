// FAS_Ezi_SERVO.h:
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FAS_EZI_SERVO_H__9185E608_0EDA_4C3B_8E92_B518968D037B__INCLUDED_)
#define AFX_FAS_EZI_SERVO_H__9185E608_0EDA_4C3B_8E92_B518968D037B__INCLUDED_


#ifdef EZI_SERVO_PLUSR_EXPORTS
	#define EZI_SERVO_PLUSR_API __declspec(dllexport)

	#include "../MotionCommon/MOTION_DEFINE.h"
#else
	#define EZI_SERVO_PLUSR_API __declspec(dllimport)
	#pragma comment( lib, "Lib/EziMOTIONPlusR.lib" )

	#include "ReturnCodes_Define.h"
	#include "MOTION_DEFINE.h"
	#include "COMM_Define.h"
#endif

#define FAPI extern "C" EZI_SERVO_PLUSR_API

FAPI BOOL WINAPI	FAS_Connect(BYTE nPortNo, DWORD dwBaud);
FAPI void WINAPI	FAS_Close(BYTE nPortNo);

FAPI void WINAPI	FAS_EnableLog(BOOL bEnable);
FAPI BOOL WINAPI	FAS_SetLogPath(LPCTSTR lpPath);

FAPI BOOL WINAPI	FAS_IsSlaveExist(BYTE nPortNo, BYTE iSlaveNo);

FAPI int WINAPI	FAS_GetSlaveInfo(BYTE nPortNo, BYTE iSlaveNo, BYTE* pType, LPSTR lpBuff, int nBuffSize);
FAPI int WINAPI	FAS_GetMotorInfo(BYTE nPortNo, BYTE iSlaveNo, BYTE* pType, LPSTR lpBuff, int nBuffSize);
FAPI int WINAPI	FAS_GetSlaveInfoEx(BYTE nPortNo, BYTE iSlaveNo, DRIVE_INFO* lpDriveInfo);

FAPI int WINAPI	FAS_SaveAllParameters(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_SetParameter(BYTE nPortNo, BYTE iSlaveNo, BYTE iParamNo, long lParamValue);
FAPI int WINAPI	FAS_GetParameter(BYTE nPortNo, BYTE iSlaveNo, BYTE iParamNo, long* lParamValue);
FAPI int WINAPI	FAS_GetROMParameter(BYTE nPortNo, BYTE iSlaveNo, BYTE iParamNo, long* lRomParam);

//------------------------------------------------------------------------------
//					IO Functions
//------------------------------------------------------------------------------
FAPI int WINAPI	FAS_SetIOInput(BYTE nPortNo, BYTE iSlaveNo, DWORD dwIOSETMask, DWORD dwIOCLRMask);
FAPI int WINAPI	FAS_GetIOInput(BYTE nPortNo, BYTE iSlaveNo, DWORD* dwIOInput);

FAPI int WINAPI	FAS_SetIOOutput(BYTE nPortNo, BYTE iSlaveNo, DWORD dwIOSETMask, DWORD dwIOCLRMask);
FAPI int WINAPI	FAS_GetIOOutput(BYTE nPortNo, BYTE iSlaveNo, DWORD* dwIOOutput);

FAPI int WINAPI	FAS_GetIOAssignMap(BYTE nPortNo, BYTE iSlaveNo, BYTE iIOPinNo, DWORD* dwIOLogicMask, BYTE* bLevel);
FAPI int WINAPI	FAS_SetIOAssignMap(BYTE nPortNo, BYTE iSlaveNo, BYTE iIOPinNo, DWORD dwIOLogicMask, BYTE bLevel);

FAPI int WINAPI	FAS_IOAssignMapReadROM(BYTE nPortNo, BYTE iSlaveNo);

//------------------------------------------------------------------------------
//					Servo Driver Control Functions
//------------------------------------------------------------------------------		
FAPI int WINAPI	FAS_ServoEnable(BYTE nPortNo, BYTE iSlaveNo, BOOL bOnOff);
FAPI int WINAPI	FAS_ServoAlarmReset(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_StepAlarmReset(BYTE nPortNo, BYTE iSlaveNo, BOOL bReset);

//------------------------------------------------------------------------------
//					Read Status and Position
//------------------------------------------------------------------------------
FAPI int WINAPI	FAS_GetAxisStatus(BYTE nPortNo, BYTE iSlaveNo, DWORD* dwAxisStatus);
FAPI int WINAPI	FAS_GetIOAxisStatus(BYTE nPortNo, BYTE iSlaveNo, DWORD* dwInStatus, DWORD* dwOutStatus, DWORD* dwAxisStatus);
FAPI int WINAPI	FAS_GetMotionStatus(BYTE nPortNo, BYTE iSlaveNo, long* lCmdPos, long* lActPos, long* lPosErr, long* lActVel, WORD* wPosItemNo);
FAPI int WINAPI	FAS_GetAllStatus(BYTE nPortNo, BYTE iSlaveNo, DWORD* dwInStatus, DWORD* dwOutStatus, DWORD* dwAxisStatus, long* lCmdPos, long* lActPos, long* lPosErr, long* lActVel, WORD* wPosItemNo);

FAPI int WINAPI	FAS_SetCommandPos(BYTE nPortNo, BYTE iSlaveNo, long lCmdPos);
FAPI int WINAPI	FAS_SetActualPos(BYTE nPortNo, BYTE iSlaveNo, long lActPos);
FAPI int WINAPI	FAS_ClearPosition(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_GetCommandPos(BYTE nPortNo, BYTE iSlaveNo, long* lCmdPos);
FAPI int WINAPI	FAS_GetActualPos(BYTE nPortNo, BYTE iSlaveNo, long* lActPos);
FAPI int WINAPI	FAS_GetPosError(BYTE nPortNo, BYTE iSlaveNo, long* lPosErr);
FAPI int WINAPI	FAS_GetActualVel(BYTE nPortNo, BYTE iSlaveNo, long* lActVel);

FAPI int WINAPI	FAS_GetAlarmType(BYTE nPortNo, BYTE iSlaveNo, BYTE* nAlarmType);

FAPI int WINAPI	FAS_GetAllTorqueStatus(BYTE nPortNo, BYTE iSlaveNo, DWORD* dwInStatus, DWORD* dwOutStatus, DWORD* dwAxisStatus, long* lCmdPos, long* lActPos, long* lPosErr, long* lActVel, WORD* wPosItemNo, WORD* wTorqueValue);
FAPI int WINAPI	FAS_GetTorqueStatus(BYTE nPortNo, BYTE iSlaveNo, WORD* wTorqueValue);

//------------------------------------------------------------------
//					Motion Functions.
//------------------------------------------------------------------
FAPI int WINAPI	FAS_MoveStop(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_EmergencyStop(BYTE nPortNo, BYTE iSlaveNo);

FAPI int WINAPI	FAS_MovePause(BYTE nPortNo, BYTE iSlaveNo, BOOL bPause);

FAPI int WINAPI	FAS_MoveOriginSingleAxis(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_MoveSingleAxisAbsPos(BYTE nPortNo, BYTE iSlaveNo, long lAbsPos, DWORD lVelocity);
FAPI int WINAPI	FAS_MoveSingleAxisIncPos(BYTE nPortNo, BYTE iSlaveNo, long lIncPos, DWORD lVelocity);
FAPI int WINAPI	FAS_MoveToLimit(BYTE nPortNo, BYTE iSlaveNo, DWORD lVelocity, int iLimitDir);
FAPI int WINAPI	FAS_MoveVelocity(BYTE nPortNo, BYTE iSlaveNo, DWORD lVelocity, int iVelDir);

FAPI int WINAPI	FAS_PositionAbsOverride(BYTE nPortNo, BYTE iSlaveNo, long lOverridePos);
FAPI int WINAPI	FAS_PositionIncOverride(BYTE nPortNo, BYTE iSlaveNo, long lOverridePos);
FAPI int WINAPI	FAS_VelocityOverride(BYTE nPortNo, BYTE iSlaveNo, DWORD lVelocity);

FAPI int WINAPI	FAS_MoveLinearAbsPos(BYTE nPortNo, BYTE nNoOfSlaves, BYTE* iSlavesNo, long* lAbsPos, DWORD lFeedrate, WORD wAccelTime);
FAPI int WINAPI	FAS_MoveLinearIncPos(BYTE nPortNo, BYTE nNoOfSlaves, BYTE* iSlavesNo, long* lIncPos, DWORD lFeedrate, WORD wAccelTime);

FAPI int WINAPI	FAS_TriggerOutput_RunA(BYTE nPortNo, BYTE iSlaveNo, BOOL bStartTrigger, long lStartPos, DWORD dwPeriod, DWORD dwPulseTime);
FAPI int WINAPI	FAS_TriggerOutput_Status(BYTE nPortNo, BYTE iSlaveNo, BYTE* bTriggerStatus);

FAPI int WINAPI	FAS_MovePush(BYTE nPortNo, BYTE iSlaveNo, DWORD dwStartSpd, DWORD dwMoveSpd, long lPosition, WORD wAccel, WORD wDecel, WORD wPushRate, DWORD dwPushSpd, long lEndPosition, WORD wPushMode);
FAPI int WINAPI	FAS_GetPushStatus(BYTE nPortNo, BYTE iSlaveNo, BYTE* nPushStatus);

//------------------------------------------------------------------
//					Ex-Motion Functions.
//------------------------------------------------------------------
FAPI int WINAPI	FAS_MoveSingleAxisAbsPosEx(BYTE nPortNo, BYTE iSlaveNo, long lAbsPos, DWORD lVelocity, MOTION_OPTION_EX* lpExOption);
FAPI int WINAPI	FAS_MoveSingleAxisIncPosEx(BYTE nPortNo, BYTE iSlaveNo, long lIncPos, DWORD lVelocity, MOTION_OPTION_EX* lpExOption);
FAPI int WINAPI	FAS_MoveVelocityEx(BYTE nPortNo, BYTE iSlaveNo, DWORD lVelocity, int iVelDir, VELOCITY_OPTION_EX* lpExOption);

//------------------------------------------------------------------
//					All-Motion Functions.
//------------------------------------------------------------------
FAPI int WINAPI	FAS_AllMoveStop(BYTE nPortNo);
FAPI int WINAPI	FAS_AllEmergencyStop(BYTE nPortNo);
FAPI int WINAPI	FAS_AllMoveOriginSingleAxis(BYTE nPortNo);
FAPI int WINAPI	FAS_AllMoveSingleAxisAbsPos(BYTE nPortNo, long lAbsPos, DWORD lVelocity);
FAPI int WINAPI	FAS_AllMoveSingleAxisIncPos(BYTE nPortNo, long lIncPos, DWORD lVelocity);

//------------------------------------------------------------------
//					Position Table Functions.
//------------------------------------------------------------------
FAPI int WINAPI	FAS_PosTableReadItem(BYTE nPortNo, BYTE iSlaveNo, WORD wItemNo, LPITEM_NODE lpItem);
FAPI int WINAPI	FAS_PosTableWriteItem(BYTE nPortNo, BYTE iSlaveNo, WORD wItemNo, LPITEM_NODE lpItem);
FAPI int WINAPI	FAS_PosTableWriteROM(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_PosTableReadROM(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_PosTableRunItem(BYTE nPortNo, BYTE iSlaveNo, WORD wItemNo);

FAPI int WINAPI	FAS_PosTableReadOneItem(BYTE nPortNo, BYTE iSlaveNo, WORD wItemNo, WORD wOffset, long* lPosItemVal);
FAPI int WINAPI	FAS_PosTableWriteOneItem(BYTE nPortNo, BYTE iSlaveNo, WORD wItemNo, WORD wOffset, long lPosItemVal);

FAPI int WINAPI	FAS_PosTableSingleRunItem(BYTE nPortNo, BYTE iSlaveNo, BOOL bNextMove, WORD wItemNo);

//------------------------------------------------------------------
//					Gap Control Functions.
//------------------------------------------------------------------
FAPI int WINAPI	FAS_GapControlEnable(BYTE nPortNo, BYTE iSlaveNo, WORD wItemNo, long lGapCompSpeed, long lGapAccTime, long lGapDecTime, long lGapStartSpeed);
FAPI int WINAPI	FAS_GapControlDisable(BYTE nPortNo, BYTE iSlaveNo);
FAPI int WINAPI	FAS_IsGapControlEnable(BYTE nPortNo, BYTE iSlaveNo, BOOL* bIsEnable, WORD* wCurrentItemNo);

FAPI int WINAPI	FAS_GapControlGetADCValue(BYTE nPortNo, BYTE iSlaveNo, long* lADCValue);
FAPI int WINAPI	FAS_GapOneResultMonitor(BYTE nPortNo, BYTE iSlaveNo, BYTE* bUpdated, long* iIndex, long* lGapValue, long* lCmdPos, long* lActPos, long* lCompValue, long* lReserved);

//------------------------------------------------------------------
//					Alarm Type History Functions.
//------------------------------------------------------------------
FAPI int WINAPI	FAS_GetAlarmLogs(BYTE nPortNo, BYTE iSlaveNo, ALARM_LOG* pAlarmLog);
FAPI int WINAPI	FAS_ResetAlarmLogs(BYTE nPortNo, BYTE iSlaveNo);

#endif // !defined(AFX_FAS_EZI_SERVO_H__9185E608_0EDA_4C3B_8E92_B518968D037B__INCLUDED_)
