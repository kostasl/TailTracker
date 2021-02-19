

library(signal)
library(Rwave)

## Timeseries Median filer, with width k
medianf <- function(t,k) {n=length(t);tproc=t; k=min(k,n); for(i in (k/2):n) tproc[i]=median(t[max(1,i-k/2): min(n, i+k/2) ]);  return(tproc)}

##Fixes Lost Tracking / and Out of Range values by Filling In Gaps with Last known good Value 
clipAngleRange <- function(vAngle,lMin,lMax)
{
  if (NROW(vAngle) < 2)
    return(NA)
  
  ##Min Idx TO Start fROM IS 2 
  idxStart <- max(  min(which(!is.na(vAngle) )   ),2)
  ##Check for Errors, Occuring when all values are NA (usually a very short vector)!
  if (is.infinite(idxStart))
    return(vAngle)
  
  ##Check  Value Before the start one- As it will be propagated forward when values are missing
  if (is.na(vAngle[idxStart-1])) 
    vAngle[idxStart-1] <- lMin
  
  if (vAngle[idxStart-1] > lMax  ) 
    vAngle[idxStart-1] <- lMax
  
  if (vAngle[idxStart-1] < lMin  ) 
    vAngle[idxStart-1] <- lMin
  
  
  for (e in idxStart:NROW(vAngle))
  {
  
    if (is.na(vAngle[e])) 
      vAngle[e] <- vAngle[e-1]
    
    if (vAngle[e] == 180)
      vAngle[e] <- vAngle[e-1]
    
    #print(vAngle[e])
    if (vAngle[e] > lMax)
      vAngle[e] <- vAngle[e-1]
    
    if (vAngle[e] < lMin)
      vAngle[e] <- vAngle[e-1]
  }
  
  return(vAngle)
}



## 
#Returns F: corresponding frequencies for the constracuted wavelet scales given #Octaves and Voices
#Fc Assumes Centre Frequency For Wavelet Function (ie morlet)
getfrqscales <- function(nVoices,nOctaves,Fs,w0)
{
  a0 <- 2^(1/nVoices)
  
  #For example, assume you are using the CWT and you set your base to s0=21/12.
  #To attach physical significance to that scale, you must multiply by the sampling interval Δt, 
  #so a scale vector covering approximately four octaves with the sampling interval taken into account is sj_0 Δt j=1,2,..48. 
  #Note that the sampling interval multiplies the scales, it is not in the exponent. For discrete wavelet transforms the base scale is always 2.
  scales <- a0^seq(to=1,by=-1,from=nVoices*nOctaves)*1/Fs
  Fc <- pi/w0 ## Morlet Centre Frequency is 1/2 when w0=2*pi
  Frq <- Fc/(scales )
  
  return(Frq)
}


maxMod <- function(x){ return( which(x == max(x)) ) }

FqRank <- function(wc){
  N_MODESAMPLES <- 6; ## No of  Low Modes to Exclude
  FqRank <- which(rank(rev(wc) ) > (NROW(wc)-N_MODESAMPLES)  )  
  return(FqRank)
}

FqMedian <- function(fRank)
{
  return(median(w.Frq[fRank]))
}


FqMean <- function(fRank)
{
  return(mean(w.Frq[fRank]))
}

## Uses Wavelets to obtain the power Fq Spectrum of the tail beat in time
## w input wave 
## returns the original object w, augmented with w.cwt w.coefSq (Power) etc.
## modal Frequencies (w.FqMod) used to detect tail beat frequency
getPowerSpectrumInTime <- function(w,Fs)
{
  ##Can Test Wavelet With Artificial Signal Sample Input Signal
  #t = seq(0,1,len=Fs)
  #w = 2 * sin(2*pi*16*t)*exp(-(t-.25)^2/.001)
  #w= w + sin(2*pi*128*t)*exp(-(t-.55)^2/.001)
  #w= w + sin(2*pi*64*t)*exp(-(t-.75)^2/.001)
  #w = ts(w,deltat=1/Fs)
  N_MODESAMPLES <- 20
  w.Fs <- Fs
  w.nVoices <- 12
  w.nOctaves <- 32
  w.W0 <- 2*pi
  
  ##Remove Missing Values
  w <- na.omit(w) 
  message("continuous wavelet transform..")
  w.cwt <- cwt(w,noctave=w.nOctaves,nvoice=w.nVoices,plot=FALSE,twoD=TRUE,w0=w.W0)
  w.coefSq <- Mod(w.cwt)^2 #Power
  
  ##Set globally so FqMedian can call it 
  w.Frq <<- getfrqscales(w.nVoices,w.nOctaves,w.Fs,w.W0)
  
  ###Make Vector Of Maximum Power-Fq Per Time Unit
  vFqMed <- rep(0,NROW(w.coefSq))
  message("Calc Fq Mode - by of high rank modes median...")
  ##matrix 1 indicates rows, 2 indicates columns, c(1, 2) 
  w.FqRank <- apply(w.coefSq,1,FqRank)
  ## Calc Median Frequency As Mod
  w.FqMod <- apply(w.FqRank,2,FqMedian)
  
  #for (i in 1:NROW(w.coefSq) )
  #{
    ##Where is the Max Power at Each TimeStep?
  #  idxDomFq <- which(w.coefSq[i,NROW( w.Frq):1] == max(w.coefSq[i,NROW(w.Frq):1]))
  #  FqRank <- which(rank(w.coefSq[i,NROW(w.Frq):1] ) > (NROW(w.Frq)-N_MODESAMPLES)  )
  #  vFqMed[i] <- median(w.Frq[FqRank]) # sum(w.coefSq[i,NROW(w.Frq):1]*w.Frq)/sum(w.Frq) #/sum(w.coefSq[i,NROW(w.Frq):1]) #w.Frq[idxDomFq] #max(coefSq[i,idxDomFq]*Frq[idxDomFq]) #sum(coefSq[i,NROW(Frq):1]*Frq)/sum(Frq) #lapply(coefSq[,NROW(Frq):1],median)
  #}
 
 
  return (list(wavedata=w,nVoices=w.nVoices,nOctaves=w.nOctaves ,MorletFrequency=w.W0, cwt=w.cwt,cwtpower=w.coefSq,Frq=w.Frq,freqMode=w.FqMod,Fs=w.Fs) )
}


## w <- Object containing Filtered Tail Segment motion (Usually the Delta angles of last 2 segments combined )
##     w.cwt <- The Continuous Wavelet Transform 
#     w.nVoices
#     w.nOctaves
## returns
plotTailPowerSpectrumInTime <- function(lwlt)
{
  
  #scales <- a0^seq(to=1,by=-1,from=nVoices*nOctaves)*1/Fs
  #Fa <- 1/2 ## Morlet Centre Frequency is 1/2 when w0=2*pi
  #Frq <- Fa/(scales )
  #Frequencies = cbind(scale=scales*(1/Fs), Frq, Period = 1./Frq)
  FONTSZ_AXISLAB = 1
  FONTSZ_AXIS = 1
  Frq <- lwlt$Frq
  #
  #plot(raster((  (vTailDisp.cwt)*1/Fs ) ), )
  #print(plot.cwt(tmp,xlab="time (units of sampling interval)"))
  ##"Frequency Content Of TailBeat"
  collist<-c("#053061","#2166AC","#4393C3","#92C5DE","#D1E5F0","#F7F7F7","#FDDBC7","#F4A582","#D6604D","#B2182B","#67001F")
  ColorRamp<-colorRampPalette(collist)(10000)
  image(x=(1000*1:NROW(lwlt$cwtpower)/lwlt$Fs),y=Frq,z=lwlt$cwtpower[,NROW(Frq):1]
        ,useRaster = FALSE
        ,main = NA
        ,xlab = NA#"Time (msec)"
        ,ylab = NA
        ,cex.lab = FONTSZ_AXISLAB
        ,cex.axis=FONTSZ_AXIS
        ,ylim=c(0,40)
        ,col=ColorRamp
  )
  mtext(side = 1,padj=1,cex=FONTSZ_AXISLAB*1.2, line = 2.2, "Time (msec)", font=2 )
  mtext(side = 2,padj=-1,cex=FONTSZ_AXISLAB*1.2, line = 2.2, "Beat Frequency (Hz)", font=2 ) 
  
  #contour(coefSq,add=T)
  #plot(coefSq[,13]   ,type='l') ##Can Plot Single Scale Like So
  
}

datTail <- read.csv2('/home/kostasl/workspace/2pTailTracker/datTrack/201020_F1_7dpf_PVN_behaviour_50000_compressed.csv',header=T,sep='\t',colClasses="numeric",dec=".")


bf_tailClass <- butter(4, c(0.01,0.15),type="pass"); ##Remove DC
bf_tailClass2 <- butter(4, 0.17,type="low"); ##Remove DC - Cut-off at 60Hz (30 hz is Tailbeat fq)
vFs                   <- 470
nFrames               = NROW(vTailDisp)
colClass <- c("#FF0000","#04A022","#0000FF")

vTailDisp <-  medianf(datTail$DThetaSpine_3 + datTail$DThetaSpine_4 + datTail$DThetaSpine_5 + datTail$DThetaSpine_6 + datTail$DThetaSpine_7,vFs/30)
vTailDisp <- filtfilt(bf_tailClass2, clipAngleRange(vTailDisp,-120,120))

lwlt <- getPowerSpectrumInTime(head(vTailDisp,nFrames),vFs )##Fix Length Differences

plot(1:NROW(lwlt$freqMode)/vFs,lwlt$freqMode,type='l',xlab="Time (sec)",ylab="Tail beat Fq Mode (Hz)")
#plotTailPowerSpectrumInTime(lwlt)

plot(1:(nFrames)/vFs,head(vTailDisp,nFrames),type='l')


