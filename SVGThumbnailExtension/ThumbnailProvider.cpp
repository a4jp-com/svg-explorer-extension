#include "Common.h"
#include "ThumbnailProvider.h"
#include "gdiplus.h"
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtCore/QFile>
#include <QString>
#include "assert.h"

using namespace Gdiplus;
CThumbnailProvider::CThumbnailProvider()
{
    DllAddRef();
    m_cRef = 1;
    m_pSite = NULL;
}


CThumbnailProvider::~CThumbnailProvider()
{
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }
    DllRelease();
}


STDMETHODIMP CThumbnailProvider::QueryInterface(REFIID riid,
                                                void** ppvObject)
{
    static const QITAB qit[] = 
    {
        QITABENT(CThumbnailProvider, IInitializeWithStream),
        QITABENT(CThumbnailProvider, IThumbnailProvider),
        QITABENT(CThumbnailProvider, IObjectWithSite),
        {0},
    };
    return QISearch(this, qit, riid, ppvObject);
}


STDMETHODIMP_(ULONG) CThumbnailProvider::AddRef()
{
    LONG cRef = InterlockedIncrement(&m_cRef);
    return (ULONG)cRef;
}


STDMETHODIMP_(ULONG) CThumbnailProvider::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

STDMETHODIMP CThumbnailProvider::Initialize(IStream *pstm, 
                                            DWORD grfMode)
{
    ULONG len;
    STATSTG stat;
    if(pstm->Stat(&stat, STATFLAG_DEFAULT) != S_OK){
        return S_FALSE;
    }

    char * data = new char[stat.cbSize.QuadPart];

    if(pstm->Read(data, stat.cbSize.QuadPart, &len) != S_OK){
        return S_FALSE;
    }

    QByteArray bytes = QByteArray(data, stat.cbSize.QuadPart);

    assert(bytes.length() > 0);//
    assert(renderer.load(bytes));
    assert(renderer.defaultSize().width() != -1);
    assert(renderer.defaultSize().height() != -1);
    assert(renderer.isValid());
    return S_OK;
}


STDMETHODIMP CThumbnailProvider::GetThumbnail(UINT cx, 
                                              HBITMAP *phbmp, 
                                              WTS_ALPHATYPE *pdwAlpha)
{
	*phbmp = NULL; 
    *pdwAlpha = WTSAT_ARGB;

    int width, height;
    QSize size = renderer.defaultSize();

    if(size.width() == size.height()){
        width = cx;
        height = cx;
    } else if (size.width() > size.height()){
        width = cx;
        height = size.height() * ((double)cx / (double)size.width());
    } else {
        width = size.width() * ((double)cx / (double)size.height());
        height = cx;
    }

    QFile * f = new QFile("C:\\dev\\svg.log");
    f->open(QFile::Append);
    f->write(QString("Size: %1 \n.").arg(cx).toAscii());
    f->flush();
    f->close();

    QImage * device = new QImage(width, height, QImage::Format_ARGB32);
    device->fill(Qt::transparent);
    //QPixmap * pixmap = new QPixmap(width, height);
    QPainter * painter = new QPainter();

    assert(painter->begin(device));
    renderer.render(painter);
    painter->end();

    assert(!device->isNull());

    *phbmp = QPixmap::fromImage(*device).toWinHBITMAP(QPixmap::Alpha);
    assert(*phbmp != NULL);

    delete painter;
    delete device;

	if( *phbmp != NULL )
		return NOERROR;
	return E_NOTIMPL;

}


STDMETHODIMP CThumbnailProvider::GetSite(REFIID riid, 
                                         void** ppvSite)
{
    if (m_pSite)
    {
        return m_pSite->QueryInterface(riid, ppvSite);
    }
    return E_NOINTERFACE;
}


STDMETHODIMP CThumbnailProvider::SetSite(IUnknown* pUnkSite)
{
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }

    m_pSite = pUnkSite;
    if (m_pSite)
    {
        m_pSite->AddRef();
    }
    return S_OK;
}


STDAPI CThumbnailProvider_CreateInstance(REFIID riid, void** ppvObject)
{
    *ppvObject = NULL;

    CThumbnailProvider* ptp = new CThumbnailProvider();
    if (!ptp)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = ptp->QueryInterface(riid, ppvObject);
    ptp->Release();
    return hr;
}
