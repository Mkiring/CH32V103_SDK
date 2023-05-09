/* 2014.09.09
*****************************************
**   Copyright  (C)  W.ch  1999-2019   **
**   Web:      http://wch.cn           **
*****************************************
**  USB-flash File Interface for CH103 **
**  KEIL423, ARM Compiler 4.1          **
*****************************************
*/
/* CH103 U�������ļ�ϵͳ�ӿ�, ֧��: FAT12/FAT16/FAT32 */

//#define DISK_BASE_BUF_LEN		512	/* Ĭ�ϵĴ������ݻ�������СΪ512�ֽ�(����ѡ��Ϊ2048����4096��֧��ĳЩ��������U��),Ϊ0���ֹ�ڱ��ļ��ж��建��������Ӧ�ó�����pDISK_BASE_BUF��ָ�� */
/* �����Ҫ���ô������ݻ������Խ�ԼRAM,��ô�ɽ�DISK_BASE_BUF_LEN����Ϊ0�Խ�ֹ�ڱ��ļ��ж��建����,����Ӧ�ó����ڵ���CH103LibInit֮ǰ��������������õĻ�������ʼ��ַ����pDISK_BASE_BUF���� */

//#define NO_DEFAULT_ACCESS_SECTOR	1		/* ��ֹĬ�ϵĴ���������д�ӳ���,���������б�д�ĳ�������� */
//#define NO_DEFAULT_DISK_CONNECT		1		/* ��ֹĬ�ϵļ����������ӳ���,���������б�д�ĳ�������� */
//#define NO_DEFAULT_FILE_ENUMER		1		/* ��ֹĬ�ϵ��ļ���ö�ٻص�����,���������б�д�ĳ�������� */

#include "debug.h"
#include "ch32v10x_usb_host.h"
#include "CH32V103UFI.h"

CMD_PARAM_I	mCmdParam;						/* ������� */
#if		DISK_BASE_BUF_LEN > 0
//UINT8	DISK_BASE_BUF[ DISK_BASE_BUF_LEN ] __attribute__((at(BA_RAM+SZ_RAM/2)));	/* �ⲿRAM�Ĵ������ݻ�����,����������Ϊһ�������ĳ��� */
UINT8	DISK_BASE_BUF[ DISK_BASE_BUF_LEN ] __attribute__((aligned (4)));	/* �ⲿRAM�Ĵ������ݻ�����,����������Ϊһ�������ĳ��� */
//UINT8	DISK_FAT_BUF[ DISK_BASE_BUF_LEN ] __attribute__((aligned (4)));	/* �ⲿRAM�Ĵ���FAT���ݻ�����,����������Ϊһ�������ĳ��� */
#endif

/* ���³�����Ը�����Ҫ�޸� */

#ifndef	NO_DEFAULT_ACCESS_SECTOR		/* ��Ӧ�ó����ж���NO_DEFAULT_ACCESS_SECTOR���Խ�ֹĬ�ϵĴ���������д�ӳ���,Ȼ�������б�д�ĳ�������� */
//if ( use_external_interface ) {  // �滻U�������ײ��д�ӳ���
//    CH103vSectorSize=512;  // ����ʵ�ʵ�������С,������512�ı���,��ֵ�Ǵ��̵�������С
//    CH103vSectorSizeB=9;   // ����ʵ�ʵ�������С��λ����,512���Ӧ9,1024��Ӧ10,2048��Ӧ11
//    CH103DiskStatus=DISK_MOUNTED;  // ǿ�ƿ��豸���ӳɹ�(ֻ������ļ�ϵͳ)
//}

UINT8	CH103ReadSector( UINT8 SectCount, PUINT8 DataBuf )  /* �Ӵ��̶�ȡ������������ݵ��������� */
{
	UINT8	retry;
//	if ( use_external_interface ) return( extReadSector( CH103vLbaCurrent, SectCount, DataBuf ) );  /* �ⲿ�ӿ� */
	for( retry = 0; retry < 3; retry ++ ) {  /* �������� */
		pCBW -> mCBW_DataLen = (UINT32)SectCount << CH103vSectorSizeB;  /* ���ݴ��䳤�� */
		pCBW -> mCBW_Flag = 0x80;
		pCBW -> mCBW_LUN = CH103vCurrentLun;
		pCBW -> mCBW_CB_Len = 10;
		pCBW -> mCBW_CB_Buf[ 0 ] = SPC_CMD_READ10;
		pCBW -> mCBW_CB_Buf[ 1 ] = 0x00;
		pCBW -> mCBW_CB_Buf[ 2 ] = (UINT8)( CH103vLbaCurrent >> 24 );
		pCBW -> mCBW_CB_Buf[ 3 ] = (UINT8)( CH103vLbaCurrent >> 16 );
		pCBW -> mCBW_CB_Buf[ 4 ] = (UINT8)( CH103vLbaCurrent >> 8 );
		pCBW -> mCBW_CB_Buf[ 5 ] = (UINT8)( CH103vLbaCurrent );
		pCBW -> mCBW_CB_Buf[ 6 ] = 0x00;
		pCBW -> mCBW_CB_Buf[ 7 ] = 0x00;
		pCBW -> mCBW_CB_Buf[ 8 ] = SectCount;
		pCBW -> mCBW_CB_Buf[ 9 ] = 0x00;
		CH103BulkOnlyCmd( DataBuf );  /* ִ�л���BulkOnlyЭ������� */
		if ( CH103IntStatus == ERR_SUCCESS ) {
			return( ERR_SUCCESS );
		}
		CH103IntStatus = CH103AnalyzeError( retry );
		if ( CH103IntStatus != ERR_SUCCESS ) {
			return( CH103IntStatus );
		}
	}
	return( CH103IntStatus = ERR_USB_DISK_ERR );  /* ���̲������� */
}

#ifdef	EN_DISK_WRITE
UINT8	CH103WriteSector( UINT8 SectCount, PUINT8 DataBuf )  /* ���������еĶ�����������ݿ�д����� */
{
	UINT8	retry;
//	if ( use_external_interface ) return( extWriteSector( CH103vLbaCurrent, SectCount, DataBuf ) );  /* �ⲿ�ӿ� */
	for( retry = 0; retry < 3; retry ++ ) {  /* �������� */
		pCBW -> mCBW_DataLen = (UINT32)SectCount << CH103vSectorSizeB;  /* ���ݴ��䳤�� */
		pCBW -> mCBW_Flag = 0x00;
		pCBW -> mCBW_LUN = CH103vCurrentLun;
		pCBW -> mCBW_CB_Len = 10;
		pCBW -> mCBW_CB_Buf[ 0 ] = SPC_CMD_WRITE10;
		pCBW -> mCBW_CB_Buf[ 1 ] = 0x00;
		pCBW -> mCBW_CB_Buf[ 2 ] = (UINT8)( CH103vLbaCurrent >> 24 );
		pCBW -> mCBW_CB_Buf[ 3 ] = (UINT8)( CH103vLbaCurrent >> 16 );
		pCBW -> mCBW_CB_Buf[ 4 ] = (UINT8)( CH103vLbaCurrent >> 8 );
		pCBW -> mCBW_CB_Buf[ 5 ] = (UINT8)( CH103vLbaCurrent );
		pCBW -> mCBW_CB_Buf[ 6 ] = 0x00;
		pCBW -> mCBW_CB_Buf[ 7 ] = 0x00;
		pCBW -> mCBW_CB_Buf[ 8 ] = SectCount;
		pCBW -> mCBW_CB_Buf[ 9 ] = 0x00;
		CH103BulkOnlyCmd( DataBuf );  /* ִ�л���BulkOnlyЭ������� */
		if ( CH103IntStatus == ERR_SUCCESS ) {
			Delay_Us( 200 );  /* д��������ʱ */
			return( ERR_SUCCESS );
		}
		CH103IntStatus = CH103AnalyzeError( retry );
		if ( CH103IntStatus != ERR_SUCCESS ) {
			return( CH103IntStatus );
		}
	}
	return( CH103IntStatus = ERR_USB_DISK_ERR );  /* ���̲������� */
}
#endif
#endif  // NO_DEFAULT_ACCESS_SECTOR

#ifndef	NO_DEFAULT_DISK_CONNECT			/* ��Ӧ�ó����ж���NO_DEFAULT_DISK_CONNECT���Խ�ֹĬ�ϵļ����������ӳ���,Ȼ�������б�д�ĳ�������� */
// only one host port
#define	UHUB0_CTRL		R8_UHOST_CTRL
#define	UHUB1_CTRL		R8_UHOST_CTRL
#define	bUHS_H0_ATTACH	RB_UMS_DEV_ATTACH
#define	bUHS_H1_ATTACH	RB_UMS_DEV_ATTACH
/* �������Ƿ����� */
UINT8	CH103DiskConnect( void )
{
	R8_USB_DEV_AD &= 0x7F;
	if ( R8_USB_DEV_AD == USB_DEVICE_ADDR || R8_USB_DEV_AD == USB_DEVICE_ADDR + 1 ) {  /* ����Root-HUB�µ�USB�豸 */
		if ( ( R8_USB_DEV_AD == USB_DEVICE_ADDR ? UHUB0_CTRL : UHUB1_CTRL ) & RB_UH_PORT_EN ) {  /* ����Root-HUB�µ�USB�豸������δ��� */
			return( ERR_SUCCESS );  /* USB�豸�Ѿ�������δ��� */
		}
		else if ( R8_USB_MIS_ST & (UINT8)( R8_USB_DEV_AD == USB_DEVICE_ADDR ? bUHS_H0_ATTACH : bUHS_H1_ATTACH ) ) {  /* ����Root-HUB�µ�USB�豸���� */
mDiskConnect:
			CH103DiskStatus = DISK_CONNECT;  /* �����Ͽ��� */
			return( ERR_SUCCESS );  /* �ⲿHUB��USB�豸�Ѿ����ӻ��߶Ͽ����������� */
		}
		else {  /* USB�豸�Ͽ� */
mDiskDisconn:
			CH103DiskStatus = DISK_DISCONNECT;
			return( ERR_USB_DISCON );
		}
	}
#ifndef	FOR_ROOT_UDISK_ONLY
	else if ( (R8_USB_DEV_AD > 0x10 && R8_USB_DEV_AD <= 0x14) || (R8_USB_DEV_AD > 0x20 && R8_USB_DEV_AD <= 0x24) ) {  /* �ⲿHUB�Ķ˿��µ�USB�豸 */
		if ( ( R8_USB_DEV_AD & 0x20 ? UHUB1_CTRL : UHUB0_CTRL ) & RB_UH_PORT_EN ) {  /* ����Root-HUB�µ��ⲿHUB������δ��� */
			TxBuffer[ MAX_PACKET_SIZE - 1 ] = R8_USB_DEV_AD;  /* ���� */
			R8_USB_DEV_AD = USB_DEVICE_ADDR - 1 + ( R8_USB_DEV_AD >> 4 );  /* ����USB�����˵�USB��ַָ��HUB */
			CH103IntStatus = HubGetPortStatus( TxBuffer[ MAX_PACKET_SIZE - 1 ] & 0x0F );  /* ��ѯHUB�˿�״̬,������TxBuffer�� */
			if ( CH103IntStatus == ERR_SUCCESS ) {
				if ( TxBuffer[2] & (1<<(HUB_C_PORT_CONNECTION-0x10)) ) {  /* ��⵽HUB�˿��ϵĲ���¼� */
					CH103DiskStatus = DISK_DISCONNECT;  /* �ٶ�ΪHUB�˿��ϵ�USB�豸�Ͽ� */
					HubClearPortFeature( TxBuffer[ MAX_PACKET_SIZE - 1 ] & 0x0F, HUB_C_PORT_CONNECTION );  /* ���HUB�˿������¼�״̬ */
				}
				R8_USB_DEV_AD = TxBuffer[ MAX_PACKET_SIZE - 1 ];  /* ����USB�����˵�USB��ַָ��USB�豸 */
				if ( TxBuffer[0] & (1<<HUB_PORT_CONNECTION) ) {  /* ����״̬ */
					if ( CH103DiskStatus < DISK_CONNECT ) {
						CH103DiskStatus = DISK_CONNECT;  /* �����Ͽ��� */
					}
					return( ERR_SUCCESS );  /* USB�豸�Ѿ����ӻ��߶Ͽ����������� */
				}
				else {
					CH103DiskStatus = DISK_CONNECT;
					return( ERR_HUB_PORT_FREE );  /* HUB�Ѿ����ӵ���HUB�˿���δ���Ӵ��� */
				}
			}
			else {
				R8_USB_DEV_AD = TxBuffer[ MAX_PACKET_SIZE - 1 ];  /* ����USB�����˵�USB��ַָ��USB�豸 */
				if ( CH103IntStatus == ERR_USB_DISCON ) {
					goto mDiskDisconn;
				}
				else {
					CH103DiskStatus = DISK_CONNECT;  /* HUB����ʧ�� */
					return( CH103IntStatus );
				}
			}
		}
		else if ( R8_USB_MIS_ST & (UINT8)( R8_USB_DEV_AD & 0x20 ? bUHS_H1_ATTACH : bUHS_H0_ATTACH ) ) {  /* ����Root-HUB�µ�USB�豸����,�ⲿHUB��USB�豸�Ѿ����ӻ��߶Ͽ����������� */
			goto mDiskConnect;
		}
		else {  /* �ⲿHUB�Ͽ� */
			goto mDiskConnect;
		}
	}
#endif
	else {
		goto mDiskDisconn;
	}
}
#endif  // NO_DEFAULT_DISK_CONNECT

#ifndef	NO_DEFAULT_FILE_ENUMER			/* ��Ӧ�ó����ж���NO_DEFAULT_FILE_ENUMER���Խ�ֹĬ�ϵ��ļ���ö�ٻص�����,Ȼ�������б�д�ĳ�������� */
void xFileNameEnumer( void )			/* �ļ���ö�ٻص��ӳ��� */
{
/* ���ָ��ö�����CH103vFileSizeΪ0xFFFFFFFF�����FileOpen����ôÿ������һ���ļ�FileOpen������ñ��ص�����
   �ص�����xFileNameEnumer���غ�FileOpen�ݼ�CH103vFileSize������ö��ֱ�����������ļ�����Ŀ¼�����������ǣ�
   �ڵ���FileOpen֮ǰ����һ��ȫ�ֱ���Ϊ0����FileOpen�ص�������󣬱�������CH103vFdtOffset�õ��ṹFAT_DIR_INFO��
   �����ṹ�е�DIR_Attr�Լ�DIR_Name�ж��Ƿ�Ϊ�����ļ�������Ŀ¼������¼�����Ϣ������ȫ�ֱ�������������
   ��FileOpen���غ��жϷ���ֵ�����ERR_MISS_FILE��ERR_FOUND_NAME����Ϊ�����ɹ���ȫ�ֱ���Ϊ����������Ч�ļ�����
   ����ڱ��ص�����xFileNameEnumer�н�CH103vFileSize��Ϊ1����ô����֪ͨFileOpen��ǰ���������������ǻص��������� */
#if		0
	UINT8			i;
	UINT16			FileCount;
	PX_FAT_DIR_INFO	pFileDir;
	PUINT8			NameBuf;
	pFileDir = (PX_FAT_DIR_INFO)( pDISK_BASE_BUF + CH103vFdtOffset );  /* ��ǰFDT����ʼ��ַ */
	FileCount = (UINT16)( 0xFFFFFFFF - CH103vFileSize );  /* ��ǰ�ļ�����ö�����,CH103vFileSize��ֵ��0xFFFFFFFF,�ҵ��ļ�����ݼ� */
	if ( FileCount < sizeof( FILE_DATA_BUF ) / 12 ) {  /* ��黺�����Ƿ��㹻���,�ٶ�ÿ���ļ�����ռ��12���ֽڴ�� */
		NameBuf = & FILE_DATA_BUF[ FileCount * 12 ];  /* ���㱣�浱ǰ�ļ����Ļ�������ַ */
		for ( i = 0; i < 11; i ++ ) NameBuf[ i ] = pFileDir -> DIR_Name[ i ];  /* �����ļ���,����Ϊ11���ַ�,δ�����ո� */
//		if ( pFileDir -> DIR_Attr & ATTR_DIRECTORY ) NameBuf[ i ] = 1;  /* �ж���Ŀ¼�� */
		NameBuf[ i ] = 0;  /* �ļ��������� */
	}
#endif
}
#endif  // NO_DEFAULT_FILE_ENUMER

UINT8	CH103LibInit( void )  /* ��ʼ��CH103�����,�����ɹ�����0 */
{
	if ( CH103GetVer( ) < CH103_LIB_VER ) return( 0xFF );  /* ��ȡ��ǰ�ӳ����İ汾��,�汾̫���򷵻ش��� */
#if		DISK_BASE_BUF_LEN > 0
	pDISK_BASE_BUF = & DISK_BASE_BUF[0];  /* ָ���ⲿRAM�Ĵ������ݻ����� */
	pDISK_FAT_BUF = & DISK_BASE_BUF[0];  /* ָ���ⲿRAM�Ĵ���FAT���ݻ�����,������pDISK_BASE_BUF�����Խ�ԼRAM */
//	pDISK_FAT_BUF = & DISK_FAT_BUF[0];  /* ָ���ⲿRAM�Ĵ���FAT���ݻ�����,������pDISK_BASE_BUF������ٶ� */
/* ���ϣ������ļ���ȡ�ٶ�,��ô�������������е���CH103LibInit֮��,��pDISK_FAT_BUF����ָ����һ�������������pDISK_BASE_BUFͬ����С�Ļ����� */
#endif
	CH103DiskStatus = DISK_UNKNOWN;  /* δ֪״̬ */
	CH103vSectorSizeB = 9;  /* Ĭ�ϵ��������̵�������512B */
	CH103vSectorSize = 512;  // Ĭ�ϵ��������̵�������512B,��ֵ�Ǵ��̵�������С
	CH103vStartLba = 0;  /* Ĭ��Ϊ�Զ�����FDD��HDD */
	return( ERR_SUCCESS );
}