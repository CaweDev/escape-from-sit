/**
 * @file Texture2D.cpp
 * @brief Texture2D implementation
 */

#include "Texture2D.hpp"
#include "TextureUtils.hpp"

namespace Engine
{
    // Static member initialization
    Texture2D* Texture2D::s_whiteTexture = nullptr;

    Texture2D::Texture2D(const std::string& filePath)
    {
        Load(filePath);
    }

    Texture2D::~Texture2D()
    {
        Unload();
    }

    Texture2D::Texture2D(Texture2D&& other) noexcept
        : m_texture(other.m_texture)
        , m_filePath(std::move(other.m_filePath))
        , m_pixelWidth(other.m_pixelWidth)
        , m_pixelHeight(other.m_pixelHeight)
    {
        other.m_texture = nullptr;
        other.m_pixelWidth = 0;
        other.m_pixelHeight = 0;
    }

    Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
    {
        if (this != &other)
        {
            Unload();
            m_texture = other.m_texture;
            m_filePath = std::move(other.m_filePath);
            m_pixelWidth = other.m_pixelWidth;
            m_pixelHeight = other.m_pixelHeight;

            other.m_texture = nullptr;
            other.m_pixelWidth = 0;
            other.m_pixelHeight = 0;
        }
        return *this;
    }

    bool Texture2D::Load(const std::string& filePath)
    {
        // Unload any existing texture
        Unload();

        m_filePath = filePath;
        m_texture = AEGfxTextureLoad(filePath.c_str());

        if (m_texture)
        {
            // Query pixel dimensions via WIC
            CpuTexture temp;
            if (TextureUtils::LoadCpuPixels(filePath, temp))
            {
                m_pixelWidth = temp.width;
                m_pixelHeight = temp.height;
            }
            return true;
        }

        return false;
    }

    void Texture2D::Unload()
    {
        if (m_texture)
        {
            AEGfxTextureUnload(m_texture);
            m_texture = nullptr;
            m_filePath.clear();
            m_pixelWidth = 0;
            m_pixelHeight = 0;
        }
    }

    Texture2D* Texture2D::WhiteTexture()
    {
        if (!s_whiteTexture)
        {
            InitializeStatics();
        }
        return s_whiteTexture;
    }

    void Texture2D::InitializeStatics()
    {
        if (!s_whiteTexture)
        {
            s_whiteTexture = new Texture2D();
        }
    }

    void Texture2D::ShutdownStatics()
    {
        if (s_whiteTexture)
        {
            delete s_whiteTexture;
            s_whiteTexture = nullptr;
        }
    }

} // namespace Engine
