/*
*Function:
*Programed by:Ray_DK@163.com
*Complete date:
*Modified by:
*Modified date:
*Remarks:
*/
#ifndef __APP_LANGUAGE_H_
#define __APP_LANGUAGE_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "stm32f4xx.h"   


#define COM_PILE_DATE						    __DATE__   
#define COM_PILE_TIME						    __TIME__
#define FW_VERSION								"3D Printer"
#ifndef MINI
#define SW_VERSION								"Software Version:SC-10-Shark-v0.228r"
#else
#define SW_VERSION								"Software Version:SC-10-Shark-v0.2231mini"
#endif
#define WIFI_VERSION							"WIFI Version:SC-10-Shark-v0.05"
#define LASR_VERSION							"Laser Version:SC-10-Shark-v0.04 L"
#define CO_TD_INFO								"Shenzhen Shengma 3D Technology co. LTD"


#define	SK_NAME										"SKU-60-BASE"




#define C_Print				"打印"
#define C_Ctol				"控制"
#define C_Set				"设置"
#define C_Preheat			"预热"
#define C_Move				"移动"
#define C_Extrusion			"挤出"
#define C_Fan				"风扇"
#define C_About				"关于"
#define C_Language			"语言"
#define C_Status			"状态"
#define	C_PrintName			"打印文件名"
#define	C_Pause				"暂停"
#define C_Pursue			"继续"
#define C_Stop				"停止"
#define C_Tempertuare		"温度"
#define	C_Speed				"速度"
#define	C_PrintCtrl			"打印控制"
#define	C_PrintSpeed		"打印速度"
#define	C_SDPrint			"SD卡打印"
#define C_Back				"返回"
#define C_PrintFinish		"打印完成"
#define	C_Confirm			"确定"
#define C_Cancel			"取消"	
#define C_Leveling			"调平"
#define C_Adjust			"屏幕校准"
#define C_Load				"进料"
#define C_Unload			"出料"
#define C_Fast				"快速"
#define	C_Normal			"中速"
#define C_Slow				"慢速"
#define C_Continue			"断点续打"

#define	C_LaserCtol		"激光雕刻控制"
#define C_LaserMove		"激光雕刻移动"
#define	C_Zero				"归零"
#define C_Filament		"耗材耗尽"
#define C_autoleveling   "自动调平"
#define	C_offset         "偏移"


#define E_Print				"Print"
#define E_Ctol				"Control"
#define E_Set					"Setting"
#define E_Preheat			"Preheat"
#define E_Move				"Move"
#define E_Extrusion		"Extrusion"
#define E_Fan					"Fan"
#define E_About				"About"
#define E_Language		"Language"
#define E_Status			"Status"
#define	E_PrintName		"PrintFileName"
#define	E_Pause				"Pause"
#define E_Pursue			"Pursue"
#define E_Stop				"Stop"
#define E_Tempertuare	"Temperature"
#define	E_Speed				"Speed"
#define	E_PrintCtrl		"PrintCtrl"
#define E_PrintSpeed	"PrintSpeed"
#define	E_SDPrint			"SD_Print"
#define E_Back				"Back"
#define E_PrintFinish	"PrintFinish"
#define	E_Confirm			"Confirm"
#define E_Cancel			"Cancel"
#define E_Zero				"Zero"
#define E_Ctrl				"Ctrl"
#define E_Leveling			"Leveling"
#define E_Adjust			"Adjust"
#define E_Load				"Load"
#define E_Unload			"Unload"
#define E_Fast				"Fast"
#define	E_Normal			"Normal"
#define E_Slow				"Slow"
#define E_Continue			"Continue"


#define	E_LaserCtol		"LaserControl"
#define E_LaserMove		"LaserMove"
#define E_Filament		"Filament run out"
#define E_StopPrint		"Stop Printting?"
#define E_autoleveling   "Auto leveling"
#define	E_offset         "offset"

//寰锋枃
#define G_Print				"Drucken"
#define G_Ctol				"Kontrolle"
#define G_Set				"Einstellen"
#define G_Preheat			"Vorw盲rmen"
#define G_Move				"Bewegen"
#define G_Extrusion			"ausdr眉cken"
#define G_Fan				"Gebl盲se"
#define G_About				"脺ber"
#define G_Language			"Sprache"
#define G_Status			"Zustand"
#define	G_PrintName			"gedrucktes Dokument"
#define	G_Pause				"Pause"
#define G_Pursue			"Weiter"
#define G_Stop				"Stopp"
#define G_Tempertuare		"Temperatur"
#define	G_Speed				"Geschwindigkeit"
#define	G_PrintCtrl			"Drucken-Kontrolle"
#define G_PrintSpeed		"Drucken-Geschwindigkeit"
#define	G_SDPrint			"Drucken aus SD Karte"
#define G_Back				"Zur眉ck"
#define G_PrintFinish		"Vollenden"
#define	G_Confirm			"Best盲tigen"
#define G_Cancel			"Stornieren"
#define G_Zero				"Nullstellen"
#define G_Ctrl				"Kontrolle"
#define G_Leveling			"Nivellierung"
#define G_Adjust			"Kalibrierung"
#define G_Load				"Belastung"
#define G_Unload			"Entladen"
#define G_Fast				"Schnell"
#define	G_Normal			"Normal"
#define G_Slow				"Langsam" 
#define G_Continue			"Fortsetzen"


#define	G_LaserCtol			"Lasergravur-Kontrolle"
#define G_LaserMove			"Lasergravur-Bewegen"
#define G_Filament			"kein Verbrauchsmaterial"
#define G_StopPrint			"Drucken beenden?"
#define G_autoleveling   	"Auto Nivellierung"
#define	G_offset         	"Abweichung"

//French
#define F_Print				"Print"
#define F_Ctol				"Contr么ler"
#define F_Set				"R茅glage"
#define F_Preheat			"Pr茅chauffer"
#define F_Move				"D茅placer"
#define F_Extrusion			"Extrusion"
#define F_Fan				"Ventilateur"
#define F_About				"Sur"
#define F_Language			"Langage"
#define F_Status			"脡tat"
#define	F_PrintName			"Nom de fichier de l'impression"
#define	F_Pause				"Pause"
#define F_Pursue			"Continuar"
#define F_Stop				"Parada"
#define F_Tempertuare		"Temperatura"
#define	F_Speed				"Velocidad"
#define	F_PrintCtrl			"Control de Impression"
#define F_PrintSpeed		"Vitesse de l'impression"
#define	F_SDPrint			"Impression de SD"
#define F_Back				"Back"
#define F_PrintFinish		"Ach猫vement de l'impression"
#define	F_Confirm			"Confirmer"
#define F_Cancel			"Annuler"
#define F_Zero				"Remise 脿 z茅ro"
#define F_Ctrl				"Contr么ler"
#define F_Adjust			"Calibration"
#define F_Load				"Charge"
#define F_Unload			"D茅charger"
#define F_Fast				"Rapide"
#define	F_Normal			"Normal"
#define F_Slow				"Lent"
#define F_Continue			"Continuer"

#define	F_LaserCtol			"Contr么le de gravure 脿 laser"
#define F_LaserMove			"D茅placement de gravure 脿 laser"
#define F_Filament			"Consommables 茅puis茅s"
#define F_StopPrint			"Arr锚ter l'impression?"
#define F_Leveling			"Nivellement"
#define F_autoleveling   "Nivellement auto"
#define	F_offset         "d茅calage"

//Spanish
#define S_Print				"Print"
#define S_Ctol				"Controlar"
#define S_Set				"Configuraci贸n"
#define S_Preheat			"Precalentamiento"
#define S_Move				"Mover"
#define S_Extrusion			"Extrusi贸n"
#define S_Fan				"Ventilador"
#define S_About				"Sobre"
#define S_Language			"Lengua"
#define S_Status			"Estado"
#define	S_PrintName			"Nombre del documento de  impresi贸n"
#define	S_Pause				"Suspensi贸n"
#define S_Pursue			"Continuar"
#define S_Stop				"Parada"
#define S_Tempertuare		"Temperatura"
#define	S_Speed				"Velocidad"
#define	S_PrintCtrl			"Control de impresi贸n"
#define S_PrintSpeed		"Velocidad de impresi贸n"
#define	S_SDPrint			"Imprimir SD"
#define S_Back				"Back"
#define S_PrintFinish		"Terminaci贸n de impresi贸n"
#define	S_Confirm			"OK"
#define S_Cancel			"Cancelar"
#define S_Zero				"Reducir a cero"
#define S_Ctrl				"Controlar"
#define S_Leveling			"Nivelaci贸n"
#define S_Adjust			"Calibraci贸n"
#define S_Load				"Carga"
#define S_Unload			"Descargar"
#define S_Fast				"R谩pido"
#define	S_Normal			"Normal"
#define S_Slow				"Lento"
#define S_Continue			"Seguir"

#define	S_LaserCtol			"Control de grabado  l谩ser"
#define S_LaserMove			"Mover de grabado  l谩ser"
#define S_Filament			"No materiales para impresi贸n"
#define S_StopPrint			"驴Dejar de imprimir?"
#define S_autoleveling   "Nivelaci贸n auto"
#define	S_offset         "compensar"

//Portuguese
#define P_Print				"Print"
#define P_Ctol				"Contr么le"
#define P_Set				"Configura莽茫o"
#define P_Preheat			"Pr茅-aque莽a"
#define P_Move				"Movimento"
#define P_Extrusion			"Extru莽茫o"
#define P_Fan				"Ventilador"
#define P_About				"Sobre"
#define P_Language			"L铆ngua"
#define P_Status			"Estado"
#define	P_PrintName			"Nome do documento imprimido"
#define	P_Pause				"Pausar"
#define P_Pursue			"Continuar"
#define P_Stop				"Parar"
#define P_Tempertuare		"Temperatura"
#define	P_Speed				"Velocidade"
#define	P_PrintCtrl			"Contr么le de impress茫o"
#define P_PrintSpeed		"Velocidade de impress茫o"
#define	P_SDPrint			"Impress茫o sd"
#define P_Back				"Back"
#define P_PrintFinish		"Impress茫o terminada"
#define	P_Confirm			"OK"
#define P_Cancel			"Cancelar"
#define P_Zero				"Voltar a zero"
#define P_Ctrl				"Contr么le"
#define P_Adjust			"Calibra莽茫o"
#define P_Load				"Carga"
#define P_Unload			"Descarregar"
#define P_Fast				"R谩pido"
#define	P_Normal			"Normal"
#define P_Slow				"Lento"
#define P_Continue			"Continuar"

#define	P_LaserCtol			"Contr么le de grava莽茫o por laser"
#define P_LaserMove			"Movimento de grava莽茫o por laser"
#define P_Filament			"Esgotamento de consum铆veis"
#define P_StopPrint			"Parar de imprimir?"	
#define P_Leveling			"Nivelamento"
#define P_autoleveling   "Nivelamento auto"
#define	P_offset         "Deslocamento"

//日语
#define J_Print				"印刷"
#define J_Ctol				"コントロール"
#define J_Set				"設定"
#define J_Preheat			"予熱"
#define J_Move				"移動"
#define J_Extrusion			"押出"
#define J_Fan				"ファン"
#define J_About				"について"
#define J_Language			"言語"
#define J_Status			"状態"
#define	J_PrintName			"印刷ファイル名"
#define	J_Pause				"一時停"
#define J_Pursue			"続ける"
#define J_Stop				"停止"
#define J_Tempertuare		"温度"
#define	J_Speed				"スピード"
#define	J_PrintCtrl			"印刷コントロール"
#define J_PrintSpeed		"印刷スピード"
#define	J_SDPrint			"SDカード印刷"
#define J_Back				"戻る"
#define J_PrintFinish		"印刷完了"
#define	J_Confirm			"確定"
#define J_Cancel			"取消"
#define J_Zero				"クリア"
#define J_Ctrl				"コントロール"
#define J_Leveling			"底板調整"
#define J_Adjust			"訂正"
#define J_Load				"負荷"
#define J_Unload			"降ろす"
#define J_Fast				"速い"
#define	J_Normal			"普通"
#define J_Slow				"スロー"
#define J_Continue			"継続する"



#define	J_LaserCtol			"レーザー彫刻コントロール"
#define J_LaserMove			"レーザー彫刻移動"
#define J_Filament			"材料が足りないです"	
#define J_StopPrint			"印刷を停止しますか? "
#define J_autoleveling   	"セルフレベリング"
#define	J_offset         	"オフセット"

//俄语
#define R_Print				"Печать"
#define R_Ctol				"Проверка"
#define R_Set				"Настройки"
#define R_Preheat			"Предварительный подогрев"
#define R_Move				"Перемещение"
#define R_Extrusion			"Экструзия"
#define R_Fan				"Вентилятор"
#define R_About				"Об"
#define R_Language			"Язык"
#define R_Status			"Состояние"
#define	R_PrintName			"Печать имени файла"
#define	R_Pause				"Пауза"
#define R_Pursue			"Продолжить"
#define R_Stop				"Стоп"
#define R_Tempertuare		"Температура"
#define	R_Speed				"Скорость"
#define	R_PrintCtrl			"Управление печатью"
#define R_PrintSpeed		"Скорость печати"
#define	R_SDPrint			"Печать SD-карты"
#define R_Back				"Назад"
#define R_PrintFinish		"Завершение печати"
#define	R_Confirm			"Да"
#define R_Cancel			"Отмена"
#define R_Zero				"Сброс"
#define R_Ctrl				"Проверка"
#define R_Adjust			"屏幕校准"
#define R_Continue			"Продолжить"

#define	R_LaserCtol			"Контроль лазерной гравировки"
#define R_LaserMove			"Перемещение лазерной гравировки"
#define R_Filament			"Расходные материалы исчерпаны"

//Italian
#define I_Print				"Print"
#define I_Ctol				"Controlla"
#define I_Set				"Impostazioni"
#define I_Preheat			"Preriscalda"
#define I_Move				"Mossa"
#define I_Extrusion			"Estrusione"
#define I_Fan				"Ventilatore"
#define I_About				"Su"
#define I_Language			"linguaggio"
#define I_Status			"Stato"
#define	I_PrintName			"Filenome stampatp"
#define	I_Pause				"Pausa"
#define I_Pursue			"Continua"
#define I_Stop				"Ferma"
#define I_Tempertuare		"Temperatura"
#define	I_Speed				"Velocit脿"
#define	I_PrintCtrl			"Controllo di stampa"
#define I_PrintSpeed		"Velocit脿 di stampa"
#define	I_SDPrint			"Stampa da SD"
#define I_Back				"Back"
#define I_PrintFinish		"Fine stampa"
#define	I_Confirm			"Conferma"
#define I_Cancel			"Annulla"
#define I_Zero				"Azzera"
#define I_Ctrl				"Controlla"
#define I_Leveling			"livellamento"
#define I_Adjust			"correzione"
#define I_Load				"Caricare"
#define I_Unload			"Scaricare"
#define I_Fast				"Veloce"
#define	I_Normal			"Normale"
#define I_Slow				"Lento"
#define I_Continue			"Continua"

#define	I_LaserCtol			"Controllo dell'incisione laser"
#define I_LaserMove			"Incisione laser mobile"
#define I_Filament			"Materiale esaurito"
#define I_StopPrint			"Smetti di stampare?"	
#define I_autoleveling   "Livellamento auto"
#define	I_offset         "compensare"


//#define MSG_AUTHOR								" Kim"
#define MSG_CONTACT								"Ray_DK@163.com"
#define MSG_COTD									"http://www.KimAuto.com.cn"

//#define __TIM2_CCRV_MOTO					//????????????????
#define __TMC_DRIVER							//??TMC????????????
#define __USR_SYS_CONFIG_SPEED		//????????,???????????

#define USER_DEBUG_LEVEL 2

#if (USER_DEBUG_LEVEL > 0)

#define  USR_ErrLog(...)    printf("ERROR: ") ;\
                            printf(__VA_ARGS__);\
														printf("\r\n");
#else
#define USR_ErrorLog(...)
#endif

#if (USER_DEBUG_LEVEL > 1)
#define  USR_UsrLog(...)    printf(__VA_ARGS__);\
														printf("\r\n");
#else
#define USR_UsrLog(...)
#endif

#if(USER_DEBUG_LEVEL > 2)
#define  USR_DbgLog(...)    printf("DBUG: ");\
                            printf(__VA_ARGS__);\
                            printf("\r\n");
#else
#define USR_DbgLog(...)
#endif

/*
修改说明:
1.当前使用层来处理打印问题,在测试过程中发现有的切片软件没有总层数信息,因此使用此种办法没有办法解决打印比例问题

*/



#ifdef __cplusplus
}
#endif				//End of __cplusplus

#endif				//End of files








