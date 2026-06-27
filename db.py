from datetime import datetime
import os

from dotenv import load_dotenv
from sqlalchemy import DateTime, ForeignKey, Integer, String, Text, create_engine
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column, relationship, sessionmaker

load_dotenv()

DATABASE_URL = os.getenv("DATABASE_URL")


class Base(DeclarativeBase):
    pass


class SchedulePlan(Base):
    __tablename__ = "schedule_plans"

    id: Mapped[int] = mapped_column(Integer, primary_key=True)
    name: Mapped[str] = mapped_column(String(120), nullable=False)
    description: Mapped[str] = mapped_column(Text, default="", nullable=False)
    created_at: Mapped[datetime] = mapped_column(DateTime, default=datetime.utcnow, nullable=False)
    updated_at: Mapped[datetime] = mapped_column(
        DateTime,
        default=datetime.utcnow,
        onupdate=datetime.utcnow,
        nullable=False,
    )

    courses: Mapped[list["Course"]] = relationship(
        back_populates="plan",
        cascade="all, delete-orphan",
        order_by="Course.id",
    )


class Course(Base):
    __tablename__ = "courses"

    id: Mapped[int] = mapped_column(Integer, primary_key=True)
    plan_id: Mapped[int] = mapped_column(ForeignKey("schedule_plans.id", ondelete="CASCADE"), nullable=False)
    name: Mapped[str] = mapped_column(String(50), nullable=False)
    num: Mapped[str] = mapped_column(String(30), nullable=False)
    day: Mapped[int] = mapped_column(Integer, nullable=False)
    starttime: Mapped[str] = mapped_column(String(5), nullable=False)
    endtime: Mapped[str] = mapped_column(String(5), nullable=False)
    created_at: Mapped[datetime] = mapped_column(DateTime, default=datetime.utcnow, nullable=False)
    updated_at: Mapped[datetime] = mapped_column(
        DateTime,
        default=datetime.utcnow,
        onupdate=datetime.utcnow,
        nullable=False,
    )

    plan: Mapped[SchedulePlan] = relationship(back_populates="courses")


def is_database_configured():
    return bool(DATABASE_URL)


def get_engine():
    if not DATABASE_URL:
        raise RuntimeError("DATABASE_URL is not configured")
    return create_engine(DATABASE_URL, pool_pre_ping=True)


engine = get_engine() if DATABASE_URL else None
SessionLocal = sessionmaker(bind=engine, autoflush=False, expire_on_commit=False) if engine else None


def init_db():
    if not engine:
        raise RuntimeError("DATABASE_URL is not configured")
    Base.metadata.create_all(engine)


def get_session():
    if not SessionLocal:
        raise RuntimeError("DATABASE_URL is not configured")
    return SessionLocal()


def course_to_dict(course):
    return {
        "id": course.id,
        "name": course.name,
        "num": course.num,
        "day": course.day,
        "starttime": course.starttime,
        "endtime": course.endtime,
    }


def plan_to_summary(plan):
    return {
        "id": plan.id,
        "name": plan.name,
        "description": plan.description,
        "course_count": len(plan.courses),
        "created_at": plan.created_at.isoformat(),
        "updated_at": plan.updated_at.isoformat(),
    }


def plan_to_detail(plan):
    data = plan_to_summary(plan)
    data["courses"] = [course_to_dict(course) for course in plan.courses]
    return data
