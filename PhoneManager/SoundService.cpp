#include "stdafx.h"
#include "SoundService.h"
#include "FlyTools.h"
#include "protocol.h"

SoundService::SoundService(void)
{
	is_stop = 1;
	is_speak = 0;

	for(int i=0;i<OUT_BUF_MAX;i++){
		outBuf[i] = new char[OUT_BUF_SIZE];
	}
	for(int i=0;i<IN_BUF_MAX;i++){
		inBuf[i] = new char[IN_BUF_SIZE];
	}

	pOutFormat.wFormatTag=WAVE_FORMAT_PCM;	//simple,uncompressed format 
	pOutFormat.nChannels = 2;//1=mono, 2=stereo 
	pOutFormat.nSamplesPerSec = PCM_OUT_RATE; // 44100 
	pOutFormat.nAvgBytesPerSec = PCM_OUT_RATE * 2 * 16 / 8; 	// = nSamplesPerSec * n.Channels * wBitsPerSample/8 
	pOutFormat.wBitsPerSample = 16;	//16 for high quality, 8 for telephone-grade 
	pOutFormat.nBlockAlign = 2 * 16 / 8; // = n.Channels * wBitsPerSample/8 
	pOutFormat.cbSize=0; 
	play_sample_rate = PCM_OUT_RATE;
	
	pInFormat.wFormatTag = WAVE_FORMAT_PCM;	//simple,uncompressed format 
	pInFormat.nChannels = 1;//1=mono, 2=stereo 
	pInFormat.nSamplesPerSec = PCM_IN_RATE; // 8000 
	pInFormat.nAvgBytesPerSec = PCM_IN_RATE * 1 * 16 / 8; 	// = nSamplesPerSec * n.Channels * wBitsPerSample/8 
	pInFormat.wBitsPerSample = 16;	//16 for high quality, 8 for telephone-grade 
	pInFormat.nBlockAlign = 1 * 16 / 8; // = n.Channels * wBitsPerSample/8 
	pInFormat.cbSize = 0; 
}

SoundService::~SoundService(void)
{	
	for(int i=0;i<OUT_BUF_MAX;i++){
		delete [] outBuf[i];
	}
	for(int j=0;j<IN_BUF_MAX;j++){
		delete [] inBuf[j];
	}

	TRACE("~SoundService\n");
}

void SoundService::startPlay(int id)
{	
	this->mClientID = id;
	if(is_stop != 0){
		is_stop = 0;
		send_count = 0;
		m_recvThread = CreateThread(NULL, 0, &SoundService::socketThread, this, CREATE_SUSPENDED, NULL);  
		if (NULL!= m_recvThread) {  
		     ResumeThread(m_recvThread);  
		}
	}
}

void SoundService::switchPlay(int id)
{	
	this->mClientID = id;	
	if(play_sample_rate!=sample_rates[id]){
		pOutFormat.nSamplesPerSec = sample_rates[id]; 
		waveOutReset(hWaveOut);
		waveOutClose(hWaveOut);
		waveOutOpen(&hWaveOut, WAVE_MAPPER,&pOutFormat,0L, 0L, WAVE_FORMAT_DIRECT);
		play_sample_rate = sample_rates[id];
	}
}

DWORD CALLBACK SoundService::socketThread(LPVOID lp)
{
	TRACE("SoundService socketThread start. \n");
	SoundService *mPtr=(SoundService *)lp;
	struct sockaddr_in sin;
	struct sockaddr_in remoteAddr;	

	mPtr->sock_lis = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mPtr->sock_lis == INVALID_SOCKET)
	{
		TRACE("SoundService socket error !");
		return -1;
	} 		
	sin.sin_family = AF_INET;
	sin.sin_port = htons(mPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(mPtr->sock_lis, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		TRACE("SoundService bind error !");
		return -1;
	}
	if (listen(mPtr->sock_lis, 5) == SOCKET_ERROR)
	{
		TRACE("SoundService listen error !");
		return -1;
	}	
	int nAddrlen = sizeof(remoteAddr);
	while (mPtr->is_stop == 0)
	{
		TRACE("SoundService accept client connect, socke_cli=%d.\n",mPtr->sock_cli);
		SOCKET socket = accept(mPtr->sock_lis, (SOCKADDR *)&remoteAddr, &nAddrlen);
		TRACE("SoundService accept socke_cli=%d.\n",mPtr->sock_cli);
		if(socket != INVALID_SOCKET){
			char buff[sizeof(PC_START_PLAY)];
			memset(buff,0,sizeof(PC_START_PLAY));
			int len = recv(socket,buff,sizeof(PC_START_PLAY),0);
			int id = buff[13];
			id = (id/10-1)*7+id%10-1;
			mPtr->client_sockets[id] = socket;
			int sample_rate = ((buff[14]<<8)&0xFF00)+(byte)buff[15];
			mPtr->sample_rates[id] = sample_rate; 
			if(mPtr->m_recvThread != NULL){
				mPtr->m_recvThread = CreateThread(NULL, 0, &SoundService::recvThread, lp, CREATE_SUSPENDED, NULL);  
				if (NULL!= mPtr->m_recvThread) {  
				     ResumeThread(mPtr->m_recvThread);  
				}			
			}
			//if(mPtr->m_recordThread!=NULL){
			//	mPtr->m_recordThread = CreateThread(NULL, 0, &SoundService::recordThread, lp, CREATE_SUSPENDED, NULL);  
			//	if (NULL!= mPtr->m_recordThread) {  
			//	     ResumeThread(mPtr->m_recordThread);  
			//	}
			//}
		}
	}	
	TRACE("SoundService socketThread exit. \n"); 
	return 0;
}

DWORD CALLBACK SoundService::recvThread(LPVOID lp)
{	
	TRACE("SoundService recvThread start. \n"); 
	SoundService *mPtr=(SoundService *)lp;
	int recvLen = 0;
	int i = 0;
	MMRESULT     result; 
	result = waveOutOpen(&mPtr->hWaveOut, WAVE_MAPPER,&mPtr->pOutFormat,0L, 0L, WAVE_FORMAT_DIRECT); 
	if (result) 
	{ 
	    TRACE("SoundService Failed to open waveform output device."); 
		return -1;
	}
	while (mPtr->is_stop == 0)	{
		int ret = 0;
		if(recvLen<6){
			ret = recv(mPtr->client_sockets[mPtr->mClientID], mPtr->recv_buf+recvLen, 10240-recvLen, 0);
			if(ret<=0){
				closesocket(mPtr->client_sockets[mPtr->mClientID]);
				TRACE("SoundService recvThread exit 1. \n"); 
				return -1;
			}else{
				recvLen+=ret;
			}
		}     
		
		long allLen = 0;
		if(((byte)mPtr->recv_buf[0]==0x7e)&&((byte)mPtr->recv_buf[1]==0xa5)){
			allLen = (((byte)mPtr->recv_buf[2]<<24)&0xFF000000)+(((byte)mPtr->recv_buf[3]<<16)&0x00FF0000)+(((byte)mPtr->recv_buf[4]<<8)&0x0000FF00)+((byte)mPtr->recv_buf[5]&0x000000FF);	
			//TRACE("recv allLen=%d, len=%X%X. \n",allLen, mPtr->recv_buf[4]&0xFF,mPtr->recv_buf[5]&0xFF); 
		}else{
			TRACE("SoundService recv error recvLen=%d, head=%X,%X. \n",recvLen, mPtr->recv_buf[0]&0xFF,mPtr->recv_buf[1]&0xFF); 
			TRACE("SoundService recvThread exit 2. \n");
			//return -1;
		}
		//TRACE("allLen=%d. sokcet=%d.\n",allLen,socket);
		while (recvLen<(allLen+4)){
			int ret = recv(mPtr->client_sockets[mPtr->mClientID], mPtr->recv_buf+recvLen, (allLen+4-recvLen), 0);
			if(ret<=0){
				closesocket(mPtr->client_sockets[mPtr->mClientID]);
				TRACE("SoundService recvThread exit 3. \n"); 
				return -1;
			}else{
				recvLen+=ret;
			}
		}

		if(((byte)mPtr->recv_buf[8]==0x04)&&((byte)mPtr->recv_buf[9]==0x4a)) {
			int sample_rate = ((mPtr->recv_buf[14]<<8)&0xFF00)+(byte)mPtr->recv_buf[15];
			TRACE("SoundService recv start play, sample_rate=%d\n",sample_rate);	
			if(mPtr->play_sample_rate!=sample_rate){
				mPtr->pOutFormat.nSamplesPerSec = sample_rate; 
				waveOutReset(mPtr->hWaveOut);
				waveOutClose(mPtr->hWaveOut);
				waveOutOpen(&mPtr->hWaveOut, WAVE_MAPPER,&mPtr->pOutFormat,0L, 0L, WAVE_FORMAT_DIRECT);
				mPtr->play_sample_rate = sample_rate;
			}
			int len = sizeof(PC_START_PLAY);
			if(recvLen-len>0){
				memmove(mPtr->recv_buf,mPtr->recv_buf+len,recvLen-len);
			}
			recvLen-=len;
			TRACE("SoundService recv play data, recvLen=%d\n",recvLen);
			//TODO:
		}else if(((byte)mPtr->recv_buf[0]==0x7e)&&((byte)mPtr->recv_buf[1]==0xa5)&&((byte)mPtr->recv_buf[8]==0x04)&&((byte)mPtr->recv_buf[9]==0x4b)) {
			int dataLen = (((byte)mPtr->recv_buf[14]<<24)&0xFF000000)+(((byte)mPtr->recv_buf[15]<<16)&0xFF0000)+(((byte)mPtr->recv_buf[16]<<8)&0xFF00)+(byte)mPtr->recv_buf[17];
			//TRACE("recv play data, dataLen=%d\n",dataLen);
			memset(mPtr->outBuf[i],0,OUT_BUF_SIZE);
			memcpy(mPtr->outBuf[i],mPtr->recv_buf+sizeof(PC_PLAY_DATA)-2,dataLen);
			mPtr->WaveOutHdr[i].lpData = mPtr->outBuf[i];
			mPtr->WaveOutHdr[i].dwBufferLength = dataLen;
			mPtr->WaveOutHdr[i].dwUser = 0L;
			mPtr->WaveOutHdr[i].dwFlags = 0L;
			mPtr->WaveOutHdr[i].dwLoops = 0L;
			waveOutPrepareHeader(mPtr->hWaveOut, &mPtr->WaveOutHdr[i], sizeof(WAVEHDR));
			waveOutWrite(mPtr->hWaveOut, &mPtr->WaveOutHdr[i], sizeof(WAVEHDR));
			i++;	
			if(i>=OUT_BUF_MAX) i = 0;
			int len = dataLen+sizeof(PC_PLAY_DATA);
			if(recvLen-len>0){
				memmove(mPtr->recv_buf,mPtr->recv_buf+len,recvLen-len);
			}
			recvLen-=len;
		}else if(((byte)mPtr->recv_buf[0]==0x7e)&&((byte)mPtr->recv_buf[1]==0xa5)&&((byte)mPtr->recv_buf[8]==0x04)&&((byte)mPtr->recv_buf[9]==0x4c)) {
			TRACE("SoundService recv pause play.\n");	
			int len = sizeof(PC_PAUSE_PLAY);
			if(recvLen-len>0){
				memmove(mPtr->recv_buf,mPtr->recv_buf+len,recvLen-len);
			}
			recvLen-=len;
			//TODO:
		}else if(((byte)mPtr->recv_buf[0]==0x7e)&&((byte)mPtr->recv_buf[1]==0xa5)&&((byte)mPtr->recv_buf[8]==0x04)&&((byte)mPtr->recv_buf[9]==0x4d)) {
			TRACE("SoundService recv stop play.\n");	
			if(mPtr->play_sample_rate!=PCM_OUT_RATE){
				mPtr->pOutFormat.nSamplesPerSec = PCM_OUT_RATE; 
				waveOutReset(mPtr->hWaveOut);
				waveOutClose(mPtr->hWaveOut);
				waveOutOpen(&mPtr->hWaveOut, WAVE_MAPPER,&mPtr->pOutFormat,0L, 0L, WAVE_FORMAT_DIRECT);
				mPtr->play_sample_rate = PCM_OUT_RATE;
			}
			int len = sizeof(PC_STOP_PLAY);
			if(recvLen-len>0){
				memmove(mPtr->recv_buf,mPtr->recv_buf+len,recvLen-len);
			}
			recvLen-=len;
			//TODO:
		}else if(((byte)mPtr->recv_buf[0]==0x7e)&&((byte)mPtr->recv_buf[1]==0xa5)&&((byte)mPtr->recv_buf[8]==0x04)&&((byte)mPtr->recv_buf[9]==0x50)) {
			TRACE("SoundService recv open speak.\n");	
			int len = sizeof(PC_OPEN_SPEAK);
			if(recvLen-len>0){
				memmove(mPtr->recv_buf,mPtr->recv_buf+len,recvLen-len);
			}
			recvLen-=len;
			mPtr->startSpeak();
			//TODO:
		}else if(((byte)mPtr->recv_buf[0]==0x7e)&&((byte)mPtr->recv_buf[1]==0xa5)&&((byte)mPtr->recv_buf[8]==0x04)&&((byte)mPtr->recv_buf[9]==0x52)) {
			TRACE("SoundService recv close speak.\n");	
			int len = sizeof(PC_CLOSE_SPEAK);
			if(recvLen-len>0){
				memmove(mPtr->recv_buf,mPtr->recv_buf+len,recvLen-len);
			}
			recvLen-=len;
			mPtr->stopSpeak();
			//TODO:
		}else{
			TRACE("SoundService recv error set recvLen=0.\n");	
			recvLen = 0;
		}
	}	

	for(int n = 0;i<OUT_BUF_MAX;i++){
		while(waveOutUnprepareHeader(mPtr->hWaveOut,&mPtr->WaveOutHdr[i],sizeof(WAVEHDR)) == WAVERR_STILLPLAYING){
			TRACE("SoundService waveOutUnprepareHeader WAVERR_STILLPLAYING. \n"); 
			Sleep(500);
		}
	}
	waveOutReset(mPtr->hWaveOut);
    waveOutClose(mPtr->hWaveOut);
    mPtr->hWaveOut = NULL;	
	TRACE("SoundService recvThread exit. \n"); 
	return 0;
}

DWORD CALLBACK SoundService::recordThread(LPVOID lp)
{
	TRACE("SoundService recordThread start. \n"); 
	SoundService *mPtr=(SoundService *)lp;	
	MMRESULT     result; 
	
	int n=waveInGetNumDevs();
	TRACE("SoundService waveInGetNumDevs %d. \n",n); 
	
	result = waveInOpen(&mPtr->hWaveIn, WAVE_MAPPER, &mPtr->pInFormat,(DWORD)MicCallBack, (DWORD)lp, CALLBACK_FUNCTION);
	if (result!= MMSYSERR_NOERROR) 
	{ 
	    TRACE("Failed to open waveform input device."); 
		return -1;
	}
	for(int i=0;i<IN_BUF_MAX;i++){
		mPtr->WaveInHdr[i].lpData = mPtr->inBuf[i];
		mPtr->WaveInHdr[i].dwBufferLength = IN_BUF_SIZE;
		mPtr->WaveInHdr[i].dwBytesRecorded = 0;
		mPtr->WaveInHdr[i].dwUser = 0L;
		mPtr->WaveInHdr[i].dwFlags = 0L;
		mPtr->WaveInHdr[i].dwLoops = 0L;
		mPtr->WaveInHdr[i].lpNext = NULL;
		mPtr->WaveInHdr[i].reserved = 0;
		waveInPrepareHeader(mPtr->hWaveIn, &mPtr->WaveInHdr[i], sizeof(WAVEHDR));		
		waveInAddBuffer(mPtr->hWaveIn, &mPtr->WaveInHdr[i], sizeof(WAVEHDR) );		
	}
	waveInStart(mPtr->hWaveIn);
	TRACE("SoundService recordThread exit. \n"); 
    return 0;
}

DWORD SoundService::MicCallBack(HWAVEIN hWaveIn,UINT uMsg,DWORD lp,DWORD dw1,DWORD dw2)
{
	//TRACE("MicCallBack start.%d, %d\n"),dw1,dw2); 
	SoundService *mPtr=(SoundService *)lp;
	PWAVEHDR pWaveInHdr =(PWAVEHDR)dw1;
	switch(uMsg)
	{
	case MM_WIM_OPEN:
		TRACE("MM_WIM_OPEN.\n"); 
		break;
	case MM_WIM_DATA:
		if(mPtr->is_stop==0 && mPtr->is_speak == 1) {
			memcpy(mPtr->send_buf,PC_SPEAK_DATA,sizeof(PC_SPEAK_DATA)-2);
			memcpy(mPtr->send_buf+(sizeof(PC_SPEAK_DATA)-2),pWaveInHdr->lpData,IN_BUF_SIZE);
			memcpy(mPtr->send_buf+(sizeof(PC_SPEAK_DATA)-2+IN_BUF_SIZE),PC_SPEAK_DATA+(sizeof(PC_SPEAK_DATA)-2),2);
			int sendLen=send(mPtr->sock_cli, mPtr->send_buf, IN_BUF_SIZE+20, 0);
			TRACE("send sendLen=%d, number=%d, sokcet=%d.\n",sendLen,mPtr->send_count,mPtr->sock_cli);
			mPtr->send_count++;
			waveInAddBuffer(hWaveIn, pWaveInHdr, sizeof(WAVEHDR) );
		}
		break;
	case MM_WIM_CLOSE:
		TRACE("MM_WIM_CLOSE.\n"); 
		waveInReset(hWaveIn);
	    waveInClose(hWaveIn);
		break;
	default:
		break;
	}
	//TRACE("MicCallBack exit.\n"); 
	return 0;
}

void SoundService::stopPlay(void)
{	
	TRACE("SoundService stopPlay. \n"); 
	if(is_stop == 0 ){
		is_stop = 1;
		for(int i=0;i<MAX_NUM;i++){
			closesocket(client_sockets[i]);
		}
		closesocket(sock_lis);		
		waveInStop(hWaveIn);
		waveInReset(hWaveIn);
		waveInClose(hWaveIn);		
	}
}

void SoundService::startSpeak(void)
{	
	is_speak = 1;
	TRACE("SoundService recordThread start. \n"); 
	MMRESULT     result; 
	
	int n=waveInGetNumDevs();
	TRACE("SoundService waveInGetNumDevs %d. \n",n); 
	
	result = waveInOpen(&hWaveIn, WAVE_MAPPER, &pInFormat,(DWORD)MicCallBack, (DWORD)this, CALLBACK_FUNCTION);
	if (result!= MMSYSERR_NOERROR) { 
	    TRACE("SoundService Failed to open waveform input device."); 
	}else{
		for(int i=0;i<IN_BUF_MAX;i++){
			WaveInHdr[i].lpData = inBuf[i];
			WaveInHdr[i].dwBufferLength = IN_BUF_SIZE;
			WaveInHdr[i].dwBytesRecorded = 0;
			WaveInHdr[i].dwUser = 0L;
			WaveInHdr[i].dwFlags = 0L;
			WaveInHdr[i].dwLoops = 1L;
			WaveInHdr[i].lpNext = NULL;
			WaveInHdr[i].reserved = 0;
			waveInPrepareHeader(hWaveIn, &WaveInHdr[i], sizeof(WAVEHDR));		
			waveInAddBuffer(hWaveIn, &WaveInHdr[i], sizeof(WAVEHDR) );		
		}
		waveInStart(hWaveIn);
	}
}

void SoundService::stopSpeak(void)
{	
	if(is_speak == 1){
		is_speak = 0;
		waveInStop(hWaveIn);
		waveInReset(hWaveIn);
		waveInClose(hWaveIn);
	}
}